#ifndef SOUL_CORE_MODULE_H
#define SOUL_CORE_MODULE_H

#include <string>
#include "soul/core/result.h"

namespace sc {

class Module {
public:
    explicit Module(const std::string& name) : m_name(name) {}
    virtual ~Module() = default;

    virtual Result<void> init() = 0;
    virtual void cleanup() = 0;

    std::string name() const { return m_name; }

private:
    std::string m_name;
};

}

#endif