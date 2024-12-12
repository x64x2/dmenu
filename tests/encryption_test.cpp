#include <iostream>
#include <string>
// laiin
#include "../src/laiin.hpp"
using namespace laiin;
namespace laiin_crypto = laiin::crypto;


int main() {
    // 1. Alice sends bob a message that is encrypted using bob's public key
    // 2. Bob receives the encrypted message and must decrpyt the message using his own private key
    // the public key
    std::string public_key =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4IcJNHFqOofSOaZaijI8\n"
    "AvDA+cYwUEpUWSXZ+NA2AtVObX2htLa3PhSSToQVUABbwPySgaHWL2xGSr/0b0Z4\n"
    "zauCK/VGidYkx85nDsOm0YjcsDUwc5t3WrRKG5+gzqxJi9g5iLaLjVzK2iPqhCTQ\n"
    "R45XtP7XBEIimZltfFW20TPC/jLIMqqKl/tkVQ1aHxqOP9k2DPPswa2JFqXF1Lnk\n"
    "nmKLKHbmn/34CMbbPBwZssailxM+hJOa+KfTRO9nP03m8z0mvRRnxu0oZFx632L9\n"
    "QX8eTHrYwYs6svbWOWE5wmfPKBAkhr94C8ricZXf8B/PAEQrF+aWrjgsLt0xtMtm\n"
    "QQIDAQAB\n"
    "-----END PUBLIC KEY-----\n";
    // the private key
    std::string private_key =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDghwk0cWo6h9I5\n"
    "plqKMjwC8MD5xjBQSlRZJdn40DYC1U5tfaG0trc+FJJOhBVQAFvA/JKBodYvbEZK\n"
    "v/RvRnjNq4Ir9UaJ1iTHzmcOw6bRiNywNTBzm3datEobn6DOrEmL2DmItouNXMra\n"
    "I+qEJNBHjle0/tcEQiKZmW18VbbRM8L+MsgyqoqX+2RVDVofGo4/2TYM8+zBrYkW\n"
	"pcXUueSeYosoduaf/fgIxts8HBmyxqKXEz6Ek5r4p9NE72c/TebzPSa9FGfG7Shk\n"
	"XHrfYv1Bfx5MetjBizqy9tY5YTnCZ88oECSGv3gLyuJxld/wH88ARCsX5pauOCwu\n"
	"3TG0y2ZBAgMBAAECggEBALWrLTx8o+o16VhyDIITAVGTwWCYBpGAgt0a7lIPDhSe\n"
	"yPV4mHWi/YNCm9rhrmjr0VHGSziOXMJERl/HDx1WFPq80feFXwy580qj6+kbT4fs\n"
	"yDve3ZQ874a5p9jQAQoYhu2bB3ph0WqQ8SUtuFwxeUDcoIS3SfyNEnfbl6XpqKF9\n"
	"TrnDINBlWDxc9clvs/3RauJMMPjlOLmtARbBQaZJQYp3LQGRn82/IFTUFxQLBpAJ\n"
	"Q7iaPthg8Rc0rrLqScejg6sNRJXUVJiTfODCcfOCCDq4hKF9aDmp+YCK6KBN04O4\n"
	"TEc9PL9o10CTrQww2qmt/Ci76w8GxrW0VAN5ZZU2kQ0CgYEA/PYMEM+/snsuX1O3\n"
	"Nd83ubnRzf7Me2i6lRcT2OpQoSicHIKAc0VtKYww1iFL+yrFXsgYFBnmkeP4DsPK\n"
	"ZN5mm4XoHeTC4AaH1SSRVdT7nEg6GVK1NbjFtbbEQaz7eKqnnpVniDK3RE3iOLjB\n"
	"nHVWcsZJZlVJjcKAkw5vacGJ6Q8CgYEA4zmLYGXY1qpkc0EQonzBxXUkI8pwfPal\n"
	"6w2/VUdmBmLD0DVRrugnVHnGNLmNk2QXoMqN40vhgD9tdY3BMVBmoRyZLgDpEhXz\n"
	"0XlLiONRkEWsqgzYoC0IG4fSvAsFlPqrXiTg34H/s74CtJ+nLF3hO/HfSkfulM/C\n"
	"dj9OFPgHm68CgYBr4ct3iAJrbhly0lM6iH5NmTAfOGGg6CNa3kK6qgPFF3qstgNu\n"
	"JdfOdlmFmSG8dptCNvf96qXo5l6ufVXd+vOrtEowJZXu0RoxDq1k+7ZrCmqszhc2\n"
	"WB0JyG6ey9VbuvxNp85FyctbOBQYuMLppSk/Pc2j9Q+vg5ouHWPqqH3WhQKBgCF+\n"
	"8SHjwaRbd/VZiRc65uGx1AMGq7BwN6M/4o2yucKFOrJtub3b8ThNvz80fz9UCPum\n"
	"AGaaYAKk1wD2RZ18abSkX5xde/4ziD6/77edMv/elYZ34FM0cDaGvjUENu1wSmTV\n"
	"cOTh6AzaHNH9mwo6SKKqlC0CD5SWT+dYi60hpxV3AoGBAKJbh7ApcUzTvEKofrS7\n"
	"xhzvnwW44cRdHNF25KMD+xkmw/4nrmifDrt+ZT5Zfa2PPBGRNDLP79mpxBnpaNLs\n"
	"LMl2fZN7vg2xY/WurhSmQjl1OmW+wFbYU2Kfmsej1tmtaO4A9xpE5jsu+L5fmrt+\n"
	"y/gd8YWuIVtUTY/HEOWneR/i\n"
	"-----END PRIVATE KEY-----\n";
    // cipher text
    std::string cipher_text = laiin_crypto::rsa_public_encrypt(public_key, "Turtles are cool");
    std::cout << "message (encrypted): " << cipher_text << std::endl;
    // plain text
    std::string plain_text = laiin_crypto::rsa_private_decrypt(private_key, cipher_text);
    std::cout << "message (decrypted): " << plain_text << std::endl;

//Example 2:
    // write plain text to file in cipher text form
    std::ofstream wfile ("cipher_text.txt", std::ios::binary);
    laiin_crypto::rsa_public_encrypt_fp(public_key, "Monero is the king of privacy coins!", wfile);
    // read cipher text from file in plain text form
    std::ifstream rfile ("cipher_text.txt", std::ios::binary);
    std::string plain_text;
    laiin_crypto::rsa_private_decrypt_fp(private_key, plain_text, rfile);
    std::cout << "message (decrypted): " << plain_text << std::endl;
*/
    // todo: encrypt private keys with a password
    
    // Get private_key contents from file
    std::ifstream key_file (std::string(laiin_CONFIG_PATH + "/secret.key").c_str(), std::ios::binary);
    std::stringstream private_key;
    private_key << key_file.rdbuf(); // dump file contents
    key_file.close();
    std::cout << "secret.key contents: " << private_key.str() << std::endl;    
    // Get SHA256sum of private_key contents
    std::string sha256sum;
    Validator::generate_sha256_hash(private_key.str(), sha256sum); // 1.7 kilobytes
    std::cout << "sha256sum of secret.key content: " << sha256sum << std::endl;*/
    return 0;
}
