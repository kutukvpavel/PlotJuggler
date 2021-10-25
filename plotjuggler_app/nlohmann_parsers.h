#ifndef NLOHMANN_PARSERS_H
#define NLOHMANN_PARSERS_H

#include "nlohmann/json.hpp"
#include "PlotJuggler/messageparser_base.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QDebug>
#include <qgroupbox.h>
#include <qlayout.h>
#include <tuple>

using namespace PJ;

typedef std::tuple<bool, std::string> params_type;

class NlohmannParser : public MessageParser
{
public:
  NlohmannParser(const std::string& topic_name, PlotDataMapRef& data, const params_type& params)
    : MessageParser(topic_name, data), _use_message_stamp(std::get<0>(params))
    , _key(std::get<1>(params))
  {
    _interpret_as_pointer = _key.empty() ? false : (_key[0] == '/');
    if (_interpret_as_pointer)
      _pointer = nlohmann::json::json_pointer(_key);
  }

protected:
  bool parseMessageImpl(double& timestamp);

  nlohmann::json _json;
  bool _use_message_stamp;
  bool _interpret_as_pointer;
  std::string _key;
  nlohmann::json::json_pointer _pointer;
};

class JSON_Parser : public NlohmannParser
{
public:
  JSON_Parser(const std::string& topic_name, PlotDataMapRef& data,
              const params_type params)
    : NlohmannParser(topic_name, data, params)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class CBOR_Parser : public NlohmannParser
{
public:
  CBOR_Parser(const std::string& topic_name, PlotDataMapRef& data,
              const params_type params)
    : NlohmannParser(topic_name, data, params)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class BSON_Parser : public NlohmannParser
{
public:
  BSON_Parser(const std::string& topic_name, PlotDataMapRef& data,
              const params_type params)
    : NlohmannParser(topic_name, data, params)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class MessagePack_Parser : public NlohmannParser
{
public:
  MessagePack_Parser(const std::string& topic_name, PlotDataMapRef& data,
                     const params_type params)
    : NlohmannParser(topic_name, data, params)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

//------------------------------------------

class ParserParamsWidget : public QGroupBox
{
public:
  ParserParamsWidget(QString checkbox_timestamp_text, QString placeholder_text, QString key_init_text) : QGroupBox()
  {
    QVBoxLayout* layout = new QVBoxLayout();
    _checkbox_use_timestamp = new QCheckBox(checkbox_timestamp_text);
    _textbox_key_member = new QLineEdit(key_init_text);
    _textbox_key_member->setPlaceholderText(placeholder_text);
    layout->addWidget(_checkbox_use_timestamp);
    layout->addWidget(_textbox_key_member);
    setLayout(layout);
  }
  ~ParserParamsWidget() override
  {
    qDebug() << "Destroying ParserParamsWidget";
  }

  params_type getParams()
  {
    auto txt = _textbox_key_member->text().toStdString();
    qDebug() << txt.c_str();
    params_type r = std::make_tuple(
      _checkbox_use_timestamp->isChecked(),
      txt
    );
    return r;
  }

protected:
  QCheckBox* _checkbox_use_timestamp;
  QLineEdit* _textbox_key_member;
};

class NlohmannParserCreator : public MessageParserCreator
{
public:
  NlohmannParserCreator()
  {
    _params_widget = new ParserParamsWidget("Use field [timestamp] if available",
                                            "Optional: key member name", "");
  }

  virtual QWidget* optionsWidget()
  {
    return _params_widget;
  }

protected:
  ParserParamsWidget* _params_widget;
};

class JSON_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<JSON_Parser>(topic_name, data,
                                         _params_widget->getParams());
  }
  const char* name() const override
  {
    return "JSON";
  }
};

class CBOR_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<CBOR_Parser>(topic_name, data, _params_widget->getParams());
  }
  const char* name() const override
  {
    return "CBOR";
  }
};

class BSON_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<BSON_Parser>(topic_name, data, _params_widget->getParams());
  }
  const char* name() const override
  {
    return "BSON";
  }
};

class MessagePack_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<MessagePack_Parser>(topic_name, data,
                                                _params_widget->getParams());
  }
  const char* name() const override
  {
    return "MessagePack";
  }
};

#endif  // NLOHMANN_PARSERS_H
