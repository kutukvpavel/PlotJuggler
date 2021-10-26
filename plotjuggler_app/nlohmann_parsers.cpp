#include "nlohmann_parsers.h"

#include "PlotJuggler/fmt/format.h"

bool NlohmannParser::parseMessageImpl(double& timestamp)
{
  const std::string timestamp_name = "timestamp";

  if (_use_message_stamp)
  {
    auto ts = _json.find(timestamp_name);
    if (ts != _json.end() && ts.value().is_number())
    {
      timestamp = ts.value().get<double>();
    }
  }

  auto key_member = std::string();
  if (_interpret_as_pointer)
  {
    try
    {
      auto val = _json.at(_pointer).begin().value();
      key_member = val.dump();
      if (val.is_number())
      {
        auto key_series = &(getSeries(_key));
        key_series->pushBack({ timestamp, val.get<double>() });
      }
      auto fs = fmt::format("[{{\"op\": \"remove\", \"path\": \"{0}\"}}]", _key);
      _json = _json.patch(nlohmann::json::parse(fs));
    }
    catch (const nlohmann::json::exception& ex)
    {
      qDebug() << ex.what();
    }
  }

  std::function<void(const std::string&, const nlohmann::json&)> flatten;
  int recursion_depth = 0;

  flatten = [&](const std::string& prefix, const nlohmann::json& value) {
    if (value.empty())
    {
      return;
    }

    switch (value.type())
    {
      case nlohmann::detail::value_t::array: {
        // iterate array and use index as reference string
        for (std::size_t i = 0; i < value.size(); ++i)
        {
          flatten(fmt::format("{}[{}]", prefix, i), value[i]);
        }
        break;
      }

      case nlohmann::detail::value_t::object: {
        // iterate object and use keys as reference string 
        if (!_interpret_as_pointer && !_key.empty())
        {
          auto it = value.find(_key);
          if (it != _json.end())
            key_member = it.value().dump();
        }
        for (const auto& element : value.items())
        {
          if (!_interpret_as_pointer && (element.key() == _key))
          {
            if (element.value().is_number())
            {
              auto plot_data = &(getSeries(fmt::format("{}/{}", prefix, _key)));
              plot_data->pushBack({ timestamp, element.value().get<double>() });
            }
          }
          else
          {
            try
            {
              flatten(fmt::format(key_member.empty() || (recursion_depth++ > 0 &&
                                                         _interpret_as_pointer) ?
                                      "{}/{}" :
                                      "{}/{}[{}]",
                                  prefix, element.key(), key_member),
                      element.value());
            }
            catch (const std::exception& ex)
            {
              qDebug() << ex.what();
            }
            --recursion_depth;
          }
        }
        break;
      }

      default: {
        double numeric_value = 0;
        if (value.is_boolean())
        {
          numeric_value = value.get<bool>();
        }
        else if (value.is_number())
        {
          numeric_value = value.get<double>();
        }
        else
        {
          return;
        }

        auto plot_data = &(getSeries(prefix));
        plot_data->pushBack({ timestamp, numeric_value });

        break;
      }
    }  // end switch
  };

  flatten(_topic_name, _json);
  return true;
}

bool MessagePack_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::from_msgpack(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}

bool JSON_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::parse(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}

bool CBOR_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::from_cbor(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}

bool BSON_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::from_bson(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}