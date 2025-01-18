#include "UnitTests/LZSSTests.h"

int main() {
    Assert(SPMEditor::Testing::TestLZSSCompression(), "U8 Failed compression test");
}
