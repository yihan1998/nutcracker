#ifndef _BACKENDS_DOCA_TARGET_H_
#define _BACKENDS_DOCA_TARGET_H_

#include <string>

#include "backends/codeGen.h"

class Target {
protected:
    explicit Target(std::string name) : name(name) {}
    Target() = delete;
    virtual ~Target() {}

public:
    const std::string name;

    // virtual void emitCodeSection(SourceCodeBuilder *builder, std::string sectionName) const = 0;
    virtual void emitIncludes(CodeBuilder *builder) const = 0;
    // virtual void emitMain(SourceCodeBuilder *builder, std::string functionName, std::string argName) const = 0;
    // virtual std::string dataOffset(std::string base) const = 0;
    // virtual std::string dataEnd(std::string base) const = 0;
    // virtual std::string dataLength(std::string base) const = 0;
    // virtual std::string forwardReturnCode() const = 0;
    // virtual std::string dropReturnCode() const = 0;
    // virtual std::string abortReturnCode() const = 0;
    // virtual void emitPreamble(SourceCodeBuilder *builder) const;
};

class DocaTarget : public Target {
public:
    DocaTarget() : Target("DOCA") {}

private:
    // void emitCodeSection(SourceCodeBuilder *, std::string) const override {}
    void emitIncludes(CodeBuilder *builder) const override;

    // void emitMain(SourceCodeBuilder *builder, std::string functionName, std::string argName) const override {
    //     builder->appendFormat("int %s(struct __sk_buff* %s)", functionName.c_str(), argName.c_str());
    // }
};

class CTarget : public Target {
public:
    CTarget() : Target("C") {}

private:
    // void emitCodeSection(SourceCodeBuilder *, std::string) const override {}
    void emitIncludes(CodeBuilder *builder) const override;

    // void emitMain(SourceCodeBuilder *builder, std::string functionName, std::string argName) const override {
    //     builder->appendFormat("int %s(struct __sk_buff* %s)", functionName.c_str(), argName.c_str());
    // }
};

#endif  /* _BACKENDS_DOCA_TARGET_H_ */