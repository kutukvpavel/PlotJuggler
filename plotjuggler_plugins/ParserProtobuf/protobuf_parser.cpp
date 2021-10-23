#include <QSettings>
#include <QMessageBox>
#include "protobuf_parser.h"
#include "PlotJuggler/fmt/format.h"
#include "PlotJuggler/svg_util.h"


using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool ProtobufParser::parseMessage(const MessageRef serialized_msg,
                                  double &timestamp)
{
  const google::protobuf::Message* prototype_msg =
      _msg_factory.GetPrototype(_msg_descriptor);

  google::protobuf::Message* mutable_msg = prototype_msg->New();
  if (!mutable_msg->ParseFromArray(serialized_msg.data(),
                                   serialized_msg.size()))
  {
    return false;
  }

  std::function<void(const google::protobuf::Message&, const std::string&)> ParseImpl;

  ParseImpl = [&](const google::protobuf::Message& msg, const std::string& prefix)
  {
    const Reflection* reflection = msg.GetReflection();
    std::vector<const FieldDescriptor*> fields;
    reflection->ListFields(msg, &fields);

    for (auto field: fields)
    {
      std::string key = prefix.empty() ?
                            field->name():
                            fmt::format("{}/{}", prefix, field->name() );
      std::string suffix;

      if (!field)
      {
        continue;
      }

      int count = 1;
      bool repeated = false;
      if (field->is_repeated())
      {
        count = reflection->FieldSize(msg, field);
        repeated = true;
      }

      for(int index = 0; index < count ; index++)
      {
        if(repeated)
        {
          suffix = fmt::format("[{}]", index);
        }

        bool is_double = true;
        double value = 0;
        switch(field->cpp_type())
        {
          case FieldDescriptor::CPPTYPE_DOUBLE:{
            value = !repeated ? reflection->GetDouble(msg, field) :
                                reflection->GetRepeatedDouble(msg, field, index);
          }break;
          case FieldDescriptor::CPPTYPE_FLOAT:{
            auto tmp = !repeated ? reflection->GetFloat(msg, field) :
                                   reflection->GetRepeatedFloat(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT32:{
            auto tmp = !repeated ? reflection->GetUInt32(msg, field) :
                                   reflection->GetRepeatedUInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT64:{
            auto tmp = !repeated ? reflection->GetUInt64(msg, field) :
                                   reflection->GetRepeatedUInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_BOOL:{
            auto tmp = !repeated ? reflection->GetBool(msg, field) :
                                   reflection->GetRepeatedBool(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT32:{
            auto tmp = !repeated ? reflection->GetInt32(msg, field) :
                                   reflection->GetRepeatedInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT64:{
            auto tmp = !repeated ? reflection->GetInt64(msg, field) :
                                   reflection->GetRepeatedInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_ENUM:{
            auto tmp = !repeated ? reflection->GetEnum(msg, field) :
                                   reflection->GetRepeatedEnum(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp->name()});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_STRING:{
            auto tmp = !repeated ? reflection->GetString(msg, field) :
                                   reflection->GetRepeatedString(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_MESSAGE:
          {
            const auto& new_msg = reflection->GetMessage(msg,field);
            ParseImpl(new_msg, key + suffix);
            is_double = false;
          }break;
        }

        if( !is_double )
        {
          auto& series = this->getSeries(key + suffix);
          series.pushBack({timestamp, value});
        }
      }
    }
  };

  // start recursion
  ParseImpl(*mutable_msg, _topic_name);

  return true;
}

ProtobufParserCreator::ProtobufParserCreator()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::ProtobufLoader;
  ui->setupUi(_widget);

  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  ui->pushButtonRemove->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));

  connect( ui->pushButtonLoad, &QPushButton::clicked, this, &ProtobufParserCreator::onLoadFile);

  auto tmp_map = settings.value("ProtobufParserCreator.protoMap").toMap();
  QMapIterator<QString, QVariant> it(tmp_map);
  while (it.hasNext())
  {
    it.next();
    getFileDescriptor( it.value().toByteArray(), it.key() );
  }

  connect( ui->listWidget, &QListWidget::currentRowChanged,
          this, &ProtobufParserCreator::onSelectionChanged );

  QString last_proto = settings.value("ProtobufParserCreator.lastProtoSelection").toString();

  auto proto_items = ui->listWidget->findItems(last_proto, Qt::MatchExactly);
  if( !last_proto.isEmpty() && proto_items.size() == 1)
  {
    ui->listWidget->setCurrentItem(proto_items.front());
  }
  QString last_type = settings.value("ProtobufParserCreator.lastType").toString();
  int combo_index = ui->comboBox->findText(last_type, Qt::MatchExactly);
  if( !last_type.isEmpty() && combo_index != -1)
  {
    ui->comboBox->setCurrentIndex(combo_index);
  }

  connect( ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &ProtobufParserCreator::onComboChanged );
}

ProtobufParserCreator::~ProtobufParserCreator()
{
  delete ui;
}

MessageParserPtr ProtobufParserCreator::createInstance(
    const std::string &topic_name, PlotDataMapRef &data)
{
  auto description = getDescription();
  return std::make_shared<ProtobufParser>(topic_name, data, description);
}

void ProtobufParserCreator::onLoadFile()
{
  QSettings settings;

  QString directory_path =
      settings.value("ProtobufParserCreator.loadDirectory", QDir::currentPath()).toString();

  QString file_name = QFileDialog::getOpenFileName(_widget, tr("Load StyleSheet"),
                                                  directory_path, tr("(*.proto)"));

  if (file_name.isEmpty())
  {
    return;
  }

  QFile file(file_name);
  file.open(QIODevice::ReadOnly);
  QByteArray proto_array = file.readAll();

  bool res = getFileDescriptor(proto_array, QFileInfo(file_name).baseName() );

  if(res)
  {
    directory_path = QFileInfo(file_name).absolutePath();
    settings.setValue("ProtobufParserCreator.loadDirectory", directory_path);
    QMap<QString, QVariant> tmp;
    QMapIterator<QString, QByteArray> it(_proto_files);
    while (it.hasNext())
    {
      it.next();
      tmp.insert(it.key(), it.value());
    }
    settings.setValue("ProtobufParserCreator.protoMap", tmp);
  }
}

void ProtobufParserCreator::onSelectionChanged(int row)
{
  QString filename = ui->listWidget->item(row)->text();
  ui->protoPreview->setText( _proto_files[filename] );

  ui->comboBox->clear();

  auto file_desc = _file_descriptors[filename];

  for(int i=0; i < file_desc->message_type_count(); i++)
  {
    ui->comboBox->addItem( QString::fromStdString(file_desc->message_type(i)->name()) );
  }
  QSettings settings;
  settings.setValue("ProtobufParserCreator.lastProtoSelection", filename);
}

void ProtobufParserCreator::onComboChanged(int)
{
  QSettings settings;
  settings.setValue("ProtobufParserCreator.lastType", ui->comboBox->currentText());
}

bool ProtobufParserCreator::getFileDescriptor(const QByteArray &proto, QString filename)
{
  ArrayInputStream proto_input_stream(proto.data(), proto.size());
  Tokenizer tokenizer(&proto_input_stream, nullptr);
  FileDescriptorProto file_desc_proto;

  Parser parser;
  if (!parser.Parse(&tokenizer, &file_desc_proto))
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("Error parsing the file"),
                         QMessageBox::Cancel);
    return false;
  }
  if (!file_desc_proto.has_name())
  {
    file_desc_proto.set_name(filename.toStdString());
  }

  const google::protobuf::FileDescriptor* file_desc = _pool.BuildFile(file_desc_proto);
  if (file_desc == nullptr)
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("Error getting file descriptor."),
                         QMessageBox::Cancel);
    return false;
  }

  _file_descriptors.insert( filename, file_desc );

  // add to the list if not present
  if( ui->listWidget->findItems(filename, Qt::MatchExactly).empty() )
  {
    ui->listWidget->addItem( filename );
    ui->listWidget->sortItems();
  }

  _proto_files.insert( filename, proto );
  return true;
}
