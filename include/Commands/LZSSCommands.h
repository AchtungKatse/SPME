#pragma once

#include "Commands/CommandType.h"
namespace SPMEditor {

    class LZSSCommands : public CommandGroup {
        public:
            class Decompress : public Command {
                public:
                    const std::string& GetName() const override { static const std::string mName = "Decompress"; return mName; }
                    const std::string& GetFormat() const override { static const std::string mFormat = "[Input File] [Output File]"; return mFormat; }
                    int GetParameterCount() const override { return 2; }
                    void Run(char** argv) const override;
            };

            class Compress : public Command {
                public:
                    const std::string& GetName() const override { static const std::string mName = "Compress"; return mName; }
                    const std::string& GetFormat() const override { static const std::string mFormat = "[Input File] [Output File]"; return mFormat; }
                    int GetParameterCount() const override { return 2; }
                    void Run(char** argv) const override;
            };

            const std::string& GetName() const override;
            const std::vector<Command*>& GetCommands() const override;

        private:
            const static std::vector<Command*> sCommands;
    };
}
