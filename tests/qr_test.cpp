// laiin
#include "../src/laiin.hpp"
using namespace laiin;

int main() {
    // QR Test
    QrCode::Ecc ecc = QrCode::Ecc::LOW;
    std::string text = "888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H";
    int image_size = 400;
    auto qr_code = qrcodegen::QrCode::encodeText(text.c_str(), ecc);    
    laiin::QR qr("qr.png", 400, text, true, ecc);
    
    std::cout << "qr image size: " << qr.get_size() << std::endl;//(qr_code) << std::endl;
    std::cout << "qr text: " << qr.get_text() << std::endl;
    std::cout << "qr file: " << qr.get_file() << std::endl;
    // Do the conversion
    qr.to_png();*/
    // deprecated function :(
    QR::export_png("qr_code.png", 400, "Turtles are cool!", true, QrCode::Ecc::LOW);

    return 0;
}
