#include <iostream>

extern "C" {
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
}
using namespace std;
int main() {
    cout << "result" << endl;
    cout << avcodec_configuration();
    return 0;
}