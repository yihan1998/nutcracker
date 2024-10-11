#ifndef _UTIL_SOURCECODEBUILDER_H_
#define _UTIL_SOURCECODEBUILDER_H_

#include <ctype.h>

#include "absl/strings/cord.h"
#include "absl/strings/str_format.h"

class SourceCodeBuilder {
    int indentLevel;  // current indent level
    unsigned indentAmount;

    absl::Cord buffer;
    bool endsInSpace;
    bool supressSemi = false;

 public:
    SourceCodeBuilder() : indentLevel(0), indentAmount(4), endsInSpace(false) {}

    void increaseIndent() { indentLevel += indentAmount; }
    void decreaseIndent() {
        indentLevel -= indentAmount;
        if (indentLevel < 0) std::cerr << "Negative indent" << std::endl;
    }
    void newline() {
        buffer.Append("\n");
        endsInSpace = true;
    }
    void spc() {
        if (!endsInSpace) buffer.Append(" ");
        endsInSpace = true;
    }

    void appendLine(const char *str) {
        append(str);
        newline();
    }
    void append(const std::string &str) {
        if (str.empty()) return;
        endsInSpace = ::isspace(str.back());
        buffer.Append(str);
    }

    template <typename... Args>
    void appendFormat(const absl::FormatSpec<Args...> &format, Args &&...args) {
        // FIXME: Sink directly to cord
        append(absl::StrFormat(format, std::forward<Args>(args)...));
    }
    void append(unsigned u) { appendFormat("%d", u); }
    void append(int u) { appendFormat("%d", u); }

    void endOfStatement(bool addNl = false) {
        if (!supressSemi) append(";");
        supressSemi = false;
        if (addNl) newline();
    }
    void supressStatementSemi() { supressSemi = true; }

    void blockStart() {
        append("{");
        newline();
        increaseIndent();
    }

    void emitIndent() {
        buffer.Append(std::string(indentLevel, ' '));
        if (indentLevel > 0) endsInSpace = true;
    }

    void blockEnd(bool nl) {
        decreaseIndent();
        emitIndent();
        append("}");
        if (nl) newline();
    }

    std::string toString() const { return std::string(buffer); }
    void commentStart() { append("/* "); }
    void commentEnd() { append(" */"); }
    bool lastIsSpace() const { return endsInSpace; }
};

#endif  /* _UTIL_SOURCECODEBUILDER_H_*/