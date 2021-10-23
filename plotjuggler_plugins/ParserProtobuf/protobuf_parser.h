#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/parser.h>

#include <QCheckBox>
#include <QDebug>
#include <string>

#include "ui_protobuf_parser.h"

using namespace PJ;

class ProtobufParser : public MessageParser
{
public:
  ProtobufParser(const std::string& topic_name, PlotDataMapRef& data,
                 const google::protobuf::Descriptor* descriptor)
    : MessageParser(topic_name, data)
    , _msg_descriptor(descriptor)
  {
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override;

protected:

  google::protobuf::DynamicMessageFactory _msg_factory;
  const google::protobuf::Descriptor* _msg_descriptor;
};

//------------------------------------------

class ProtobufParserCreator : public MessageParserCreator
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.MessageParserCreator")
  Q_INTERFACES(PJ::MessageParserCreator)

public:
  ProtobufParserCreator();

  ~ProtobufParserCreator() override;

  const char* name() const override
  {
    return "ProtobufParser";
  }

  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override;

  QWidget* optionsWidget() override
  {
    return _widget;
  }

protected:
  const google::protobuf::Descriptor* getDescription();
  Ui::ProtobufLoader* ui;
  QWidget* _widget;

  google::protobuf::DescriptorPool _pool;

  // key = filename  /  value = raw file content
  QMap<QString, QByteArray> _proto_files;

  QMap<QString, const google::protobuf::FileDescriptor*> _file_descriptors;

  bool getFileDescriptor(const QByteArray& proto, QString filename);

private slots:

  void onLoadFile();

  void onSelectionChanged(int row);

  void onComboChanged(int index);
};



