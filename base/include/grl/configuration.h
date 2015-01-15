/*
 * configuration.h
 *
 *  Created on: Nov 10, 2011
 *      Author: wcaarls
 */

#ifndef GRL_CONFIGURATION_H_
#define GRL_CONFIGURATION_H_

#include <exception>
#include <string>
#include <map>
#include <vector>

#include <sstream>
#include <fstream>

#include <grl/vector.h>

namespace grl {

template <typename T> struct type_name {
    static const char *name;
};

#define DECLARE_TYPE_NAME(x) template<> const char *type_name<x>::name = #x;

class Exception : public std::exception
{
  protected:
    std::string what_;

  public:
    Exception(std::string what) throw()
    {
      what_ = what;
    }
    virtual ~Exception() throw () { }

    virtual const char* what() const throw()
    {
      return what_.c_str();
    }
};

class bad_param : public Exception
{
  public:
    bad_param(std::string which) throw() : Exception("Parameter '" + which + "' has an illegal value") { }
};

class ConfigurationParameter
{
  protected:
    mutable std::stringstream value_;

  public:
    template <class T>
    ConfigurationParameter(T value)
    {
      value_ << value;
    }

    ConfigurationParameter(const ConfigurationParameter &other)
    {
      value_.str(other.str());
    }
    
    template <class T>
    bool get(T &value) const
    {
      value_.clear();
      value_.seekg(std::ios_base::beg);
      value_ >> value;
      return !value_.fail();
    }

    template<class T>
    operator T() const
    {
      T value;
      if (!get(value))
        throw Exception("Parameter type mismatch while converting '" + value_.str() + "' to " + type_name<T>::name);
      return value;
    }
    
    std::string str() const
    {
      return value_.str();
    }
    
    void *ptr() const
    {
      void *value;
      if (!get(value))
        throw Exception("Parameter type mismatch while converting '" + value_.str() + "' to pointer");
      return value;
    }
};

class Configuration
{
  public:
    typedef std::map<std::string, const ConfigurationParameter*> MapType;

  protected:
    mutable MapType parameters_;

  public:
    Configuration()
    {
    }
  
    Configuration(const Configuration &other)
    {
      for (MapType::iterator ii=other.parameters_.begin(); ii != other.parameters_.end(); ++ii)
        parameters_[ii->first] = new ConfigurationParameter(*ii->second);
    }
  
    ~Configuration()
    {
      for (MapType::iterator ii=parameters_.begin(); ii != parameters_.end(); ++ii)
        delete ii->second;
    }
    
    Configuration &operator=(const Configuration &other)
    {
      if (&other != this)
      {
        for (MapType::iterator ii=parameters_.begin(); ii != parameters_.end(); ++ii)
          delete ii->second;
        parameters_.clear();
      
        for (MapType::iterator ii=other.parameters_.begin(); ii != other.parameters_.end(); ++ii)
          parameters_[ii->first] = new ConfigurationParameter(*ii->second);
      }
        
      return *this;
    }

    bool has (const std::string &key) const
    {
      return parameters_.count(key) > 0;
    }

    template<class T>
    bool get (const std::string &key, T &value) const
    {
      return get(key, value, value);
    }

    template<class T>
    bool get (const std::string &key, T &value, const T &dflt) const
    {
      if (has(key))
      {
        if (!parameters_[key]->get(value))
          throw Exception("Parameter type mismatch for '" + key + "'");

        return true;
      }
      else
      {
        value = dflt;
        return false;
      }
    }

    bool get (const std::string &key, std::string &value, const char *dflt) const
    {
      if (has(key))
      {
        if (!parameters_[key]->get(value))
          throw Exception("Parameter type mismatch for '" + key + "'");

        return true;
      }
      else
      {
        value = dflt;
        return false;
      }
    }

    const ConfigurationParameter& operator[](const std::string &key) const
    {
      if (!has(key))
        throw Exception("Parameter '" + key + "' not set");

      return *parameters_[key];
    }

    template<class T>
    void set (const std::string &key, const T &value)
    {
      if (has(key))
        delete parameters_[key];
      parameters_[key] = new ConfigurationParameter(value);
    }
    
    const MapType &parameters() const
    {
      return parameters_;
    }
};

}

#endif /* GRL_CONFIGURATION_H_ */
