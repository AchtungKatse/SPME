#pragma once

namespace SPMEditor {
    class TPLCommands { 
        public:
            TPLCommands() = delete;
            static void Read(int& i, int argc, char** argv);
            static void Dump(const char* inputFile, const char* outputDirectory);

        private:
    };
}
