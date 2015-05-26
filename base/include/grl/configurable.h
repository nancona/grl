/** \file configurable.h
 * \brief Configurable object definition.
 *
 * \author    Wouter Caarls <wouter@caarls.org>
 * \date      2015-01-22
 *
 * \copyright \verbatim
 * Copyright (c) 2015, Wouter Caarls
 * All rights reserved.
 *
 * This file is part of GRL, the Generic Reinforcement Learning library.
 *
 * GRL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * \endverbatim
 */
#ifndef GRL_CONFIGURABLE_H_
#define GRL_CONFIGURABLE_H_

#include <limits.h>
#include <float.h>

#include <yaml-cpp/yaml.h>

#include <grl/utils.h>
#include <grl/configuration.h>
#include <grl/factory.h>

namespace grl {

/// Parameter requested by a Configurable object.
struct CRP
{
  typedef enum {Provided, System, Configuration, Online} Mutability;

  std::string name, type, description, value;
  Mutability mutability;
  double min, max;
  std::vector<std::string> options;
  bool optional;
  
  CRP(std::string _name, std::string _type, std::string _description,
      class Configurable *_value, bool _optional=false) :
    name(_name), type(_type), description(_description), mutability(Configuration), optional(_optional)
  {
    setValue(_value);
  }
  
  CRP(std::string _name, std::string _description,
      int _value, Mutability _mutability=Configuration, int _min=0, int _max=INT_MAX) :
    name(_name), type("int"), description(_description), mutability(_mutability), min(_min), max(_max), optional(true)
  {
    setValue(_value);
  }

  CRP(std::string _name, std::string _type, std::string _description,
      int _value, Mutability _mutability=Configuration, int _min=0, int _max=INT_MAX) :
    name(_name), type(_type), description(_description), mutability(_mutability), min(_min), max(_max), optional(true)
  {
    setValue(_value);
  }
  
  CRP(std::string _name, std::string _description,
      double _value, Mutability _mutability=Configuration, double _min=0., double _max=1.) :
    name(_name), type("double"), description(_description), mutability(_mutability), min(_min), max(_max), optional(true)
  {
    setValue(_value);
  }
  
  CRP(std::string _name, std::string _type, std::string _description,
      double _value, Mutability _mutability=Configuration, double _min=0., double _max=1.) :
    name(_name), type(_type), description(_description), mutability(_mutability), min(_min), max(_max), optional(true)
  {
    setValue(_value);
  }
  
  CRP(std::string _name, std::string _description,
      Vector _value, Mutability _mutability=Configuration) :
    name(_name), type("vector"), description(_description), mutability(_mutability), optional(true)
  {
    setValue(_value);
  }

  CRP(std::string _name, std::string _type, std::string _description,
      Vector _value, Mutability _mutability=Configuration) :
    name(_name), type(_type), description(_description), mutability(_mutability), optional(true)
  {
    setValue(_value);
  }

  CRP(std::string _name, std::string _description,
      std::string _value, Mutability _mutability=Configuration, 
      std::vector<std::string> _options=std::vector<std::string>()) :
    name(_name), type("string"), description(_description), value(_value), mutability(_mutability), options(_options), optional(true)
  { 
    // Provided parameters have the same signature as strings... Disambiguate.
    if (mutability == Provided)
    {
      type = description;
      description = value;
      value = "";
      options = std::vector<std::string>();
    }
  }
  
  inline static void split(const std::string type, std::string *base, std::string *role)
  {
    if (type.find('.') != std::string::npos)
    {
      *base = type.substr(0, type.find('.'));
      *role = type.substr(base->size()+1);
    }
    else
    {
      *base = type;
      *role = "";
    }
  }
  
  protected:
    template<class T>
    void setValue(const T& v)
    {
      std::ostringstream oss;
      oss << v;
      value = oss.str();
    }
};

/// Set of requested parameters.
typedef std::vector<CRP> ConfigurationRequest;

#define TYPEINFO(t)\
    static std::string s_type() { return t; }\
    virtual std::string d_type() const { return t; }

extern unsigned char grl_log_verbosity__;
extern const char *grl_log_levels__[];

#define GRLLOG(l, m) do { std::ostringstream __log_oss; __log_oss << m; log(l, __log_oss); } while (0)

#define ERROR(m) GRLLOG(0, m)
#define WARNING(m) GRLLOG(1, m)
#define NOTICE(m) GRLLOG(2, m)
#define INFO(m) GRLLOG(3, m)
#define DEBUG(m) GRLLOG(4, m)
#define CRAWL(m) GRLLOG(5, m)

inline void log(unsigned char level, const std::ostringstream &oss)
{
  if (level <= grl_log_verbosity__)
  {
    if (level < 2)
      std::cerr << grl_log_levels__[level] << " " << oss.str() << "\x1B[0m" << std::endl;
    else
      std::cout << grl_log_levels__[level] << " " << oss.str() << "\x1B[0m" << std::endl;
  }
}

/// Configurable object.
class Configurable
{
  public:
    std::vector<Configurable*> children_;

  protected:
    inline void log(unsigned char level, const std::ostringstream &oss) const
    {
      if (level <= grl_log_verbosity__)
      {
        if (level < 2)
          std::cerr << grl_log_levels__[level] << " " << d_type() << ": " << oss.str() << "\x1B[0m" << std::endl;
        else
          std::cout << grl_log_levels__[level] << " " << d_type() << ": " << oss.str() << "\x1B[0m" << std::endl;
      }
    }
    
  public:
    virtual ~Configurable() { }
    
    TYPEINFO("")

    virtual void request(ConfigurationRequest * /*config*/) { }
    virtual void request(const std::string &role, ConfigurationRequest *config) { request(config); }
    virtual void configure(Configuration &/*config*/) { }
    virtual void reconfigure(const Configuration &/*config*/) { }
    
    void reset()
    {
      Configuration config;
      config.set("action", "reset");
      reconfigure(config);
      
      for (size_t ii=0; ii < children_.size(); ++ii)
        children_[ii]->reset();
    }
};

DECLARE_FACTORY(Configurable)
#define REGISTER_CONFIGURABLE(subx) REGISTER_FACTORY(Configurable, subx, subx::s_type())

/// Configure objects based on a YAML file.
class YAMLConfigurator
{
  protected:
    std::vector<Configurable*> objects_;
    Configuration references_;
    std::vector<std::string> file_;

  public:
    ~YAMLConfigurator()
    {
      for (int ii=objects_.size()-1; ii >= 0; --ii)
        delete objects_[ii];

      objects_.clear();
    }
    void populate(const Configuration &config)
    {
      references_ = config;
    }
    
    const Configuration &references() const
    {
      return references_;
    }
  
    Configurable *load(std::string file, Configuration *config, const std::string &path="")
    {
      NOTICE("Loading " << file);
      
      file_.push_back(file);
      Configurable *obj = load(YAML::LoadFile(file.c_str()), config, path);
      file_.pop_back();
      
      return obj;
    }
  
    Configurable *load(const YAML::Node &node, Configuration *config, const std::string &path);
    
  protected:
    std::string toString(const YAML::Node &node)
    {
      if (node.IsScalar())
      {
        return node.as<std::string>();
      }
      else if (node.IsSequence())
      {
        std::string value;
      
        /* Ugly hack to convert sequence back to a string */
        std::stringstream ss;
        ss << "[ ";

        for (size_t ii=0; ii < node.size(); ++ii)
        {
          value = node[ii].as<std::string>();
          ss << value;
          if (ii < node.size()-1)
            ss << ", ";
        }
        ss << " ]";
        value = ss.str();

        return value;
      }
      else
        throw Exception("Unsupported YAML node type");
    }
    
    std::string parse(const std::string &value) const;
};

}

#endif /* GRL_CONFIGURABLE_H_ */
