#pragma once

namespace SPMEditor {

    class Command {
        public:
            virtual const std::string& GetName() const = 0;
            virtual const std::string& GetFormat() const = 0;
            virtual int GetParameterCount() const = 0;
            virtual void Run(char** argv) const = 0;
    };

    class CommandGroup {
        public: 
            virtual const std::string& GetName() const = 0;
            virtual const std::vector<Command*>& GetCommands() const = 0;
    };
}
