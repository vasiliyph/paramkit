#pragma once

#include <Windows.h>

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#define PARAM_UNINITIALIZED (-1)

#define PARAM_SWITCH1 '/'
#define PARAM_SWITCH2 '-'

namespace paramkit {
    //--

    class Param {
    public:
        Param(const std::string& _argStr, bool _isRequired)
        {
            isRequired = _isRequired;
            argStr = _argStr;
            requiredParam = false;
        }

        virtual std::string valToString() = 0;
        virtual std::string type() = 0;

        virtual bool parse(char *arg) = 0;
        virtual bool isSet() = 0;

    protected:
        std::string argStr;
        
        std::string info;
        bool isRequired;

        bool requiredParam; // do you need to pass argument to this param

    friend class Params;
    };

    class IntParam : public Param {
    public:
        IntParam(const std::string& _argStr, bool _isRequired, bool _isHex = false)
            : Param(_argStr, _isRequired)
        {
            requiredParam = true;
            value = PARAM_UNINITIALIZED;
            isHex = _isHex;
        }

        virtual std::string valToString()
        {
            std::stringstream stream;
            if (isHex) {
                stream << std::hex;
            }
            else {
                stream << std::dec;
            }
            stream << value;
            return stream.str();
        }

        virtual std::string type() {
            if (isHex) {
                return "integer: hex";
            }
            return "integer: dec";
        }

        virtual bool isSet()
        {
            return value != PARAM_UNINITIALIZED;
        }

        virtual bool parse(char *arg) {
            if (!arg) return false;

            uint64_t val = 0;
            if (isHex) {
                if (sscanf(arg, "%llX", &val) == 0) {
                    sscanf(arg, "%#llX", &val);
                }
                this->value = val;
                return true;
            }
            sscanf(arg, "%d", &val);
            this->value = val;
            return true;
        }

        bool isHex;
        uint64_t value;
    };

    class StringParam : public Param {
    public:
        StringParam(const std::string& _argStr, bool _isRequired)
            : Param(_argStr, _isRequired)
        {
            requiredParam = true;
            value = "";
        }

        virtual std::string valToString()
        {
            return "\"" + value + "\"";
        }

        virtual std::string type() {
            return "string";
        }

        virtual bool isSet()
        {
            return value.length() > 0;
        }

        virtual bool parse(char *arg) {
            if(!arg) return false;

            this->value = arg;
            return true;
        }

        size_t copyToCStr(char *buf, size_t buf_max)
        {
            size_t len = value.length() + 1;
            if (len > buf_max) len = buf_max;

            memcpy(buf, value.c_str(), buf_max);
            buf[len] = '\0';
            return len;
        }

        std::string value;
    };


    class BoolParam : public Param {
    public:
        BoolParam(const std::string& _argStr, bool _isRequired)
            : Param(_argStr, _isRequired)
        {
            requiredParam = false;
            value = false;
        }

        virtual std::string type() {
            return "bool";
        }

        virtual std::string valToString()
        {
            std::stringstream stream;
            stream << std::dec;
            if (value) {
                stream << "true";
            }
            else {
                stream << "false";
            }
            return stream.str();
        }

        virtual bool isSet()
        {
            return value;
        }

        virtual bool parse(char *arg = nullptr) {
            if (!arg) {
                this->value = true;
                return true;
            }
            DWORD val = 0;
            sscanf(arg, "%d", &val);
            if (val != 0) {
                this->value = true;
            }
            else {
                this->value = false;
            }
            return true;
        }

        bool value;
    };

    class Params {
    public:
        Params();
        virtual ~Params() { releaseParams(); }

        void addParam(Param* param)
        {
            if (!param) return;
            const std::string argStr = param->argStr;
            this->myParams[argStr] = param;
        }

        bool setIntValue(const std::string& str, uint64_t val)
        {
            Param *p = getParam(str);
            if (!p) return false;

            IntParam *param = dynamic_cast<IntParam*>(p);
            if (!param) {
                return false;
            }
            param->value = val;
            return true;
        }

        bool setInfo(const std::string& str, const std::string& info)
        {
            Param *p = getParam(str);
            if (!p) return false;

            p->info = info;
            return false;
        }

        uint64_t getIntValue(const std::string& str)
        {
            std::map<std::string, Param*>::iterator itr = this->myParams.find(str);
            if (itr == this->myParams.end()) return PARAM_UNINITIALIZED;

            IntParam *param = dynamic_cast<IntParam*>(itr->second);
            if (!param) {
                return false;
            }
            return param->value;
        }

        virtual bool isSet(const std::string& str)
        {
            std::map<std::string, Param*>::iterator itr = this->myParams.find(str);
            if (itr == this->myParams.end()) return false;

            Param *param = itr->second;
            if (!param) {
                return false;
            }
            return param->isSet();
        }

        bool hasRequiredFilled()
        {
            std::map<std::string, Param*>::iterator itr;
            for (itr = myParams.begin(); itr != myParams.end(); itr++) {
                Param *param = itr->second;
                if (param->isRequired && !param->isSet()) {
                    return false;
                }
            }
            return true;
        }

        void releaseParams()
        {
            std::map<std::string, Param*>::iterator itr;
            for (itr = myParams.begin(); itr != myParams.end(); itr++) {
                Param *param = itr->second;
                delete param;
            }
            myParams.clear();
        }

        void print();
        void info(bool hilightMissing = false);
        bool parse(int argc, char* argv[]);

    protected:
        Param * getParam(const std::string &str)
        {
            std::map<std::string, Param*>::iterator itr = this->myParams.find(str);
            if (itr != this->myParams.end()) {
                return itr->second;
            }
            return nullptr;
        }

        std::map<std::string, Param*> myParams;
    };
};

