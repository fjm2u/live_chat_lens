#include "credential-store.hpp"
#if defined(__APPLE__) && !defined(COMMENTS_TEST_CREDENTIAL_STORE)
#include <Security/Security.h>
namespace comments {
static CFMutableDictionaryRef query() {
    auto q=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(q,kSecClass,kSecClassGenericPassword);
    CFDictionarySetValue(q,kSecAttrService,CFSTR("obs-comment-dock"));
    CFDictionarySetValue(q,kSecAttrAccount,CFSTR("jev-api-key"));
    return q;
}
StoredKey loadJevKey() {
    auto q=query(); CFDictionarySetValue(q,kSecReturnData,kCFBooleanTrue);
    CFTypeRef result=nullptr; const auto status=SecItemCopyMatching(q,&result); CFRelease(q);
    if(status==errSecItemNotFound) return {};
    if(status!=errSecSuccess) return {{},QString("Jevキーを読み込めません。キーチェーンを確認してください (%1)").arg(status)};
    auto data=static_cast<CFDataRef>(result);
    auto key=QString::fromUtf8(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),CFDataGetLength(data));
    CFRelease(result); return {key,{}};
}
QString saveJevKey(const QString &key) {
    if(key.isEmpty()) return {}; // Clearing the field does not delete a saved key.
    const auto bytes=key.toUtf8(); auto data=CFDataCreate(nullptr,reinterpret_cast<const UInt8 *>(bytes.constData()),bytes.size());
    auto q=query(); auto values=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(values,kSecValueData,data);
    auto status=SecItemUpdate(q,values);
    if(status==errSecItemNotFound) {
        CFDictionarySetValue(q,kSecValueData,data);
        CFDictionarySetValue(q,kSecAttrLabel,CFSTR("AI Comments — Jev API Key"));
        status=SecItemAdd(q,nullptr);
    }
    CFRelease(values); CFRelease(q); CFRelease(data);
    return status==errSecSuccess?QString{}:QString("Jevキーを保存できません。今回はメモリ内で使用します (%1)").arg(status);
}
}
#elif defined(COMMENTS_TEST_CREDENTIAL_STORE)
namespace comments {
static QString saved;
StoredKey loadJevKey() { return {saved,{}}; }
QString saveJevKey(const QString &key) { if(!key.isEmpty()) saved=key; return {}; }
}
#else
namespace comments {
StoredKey loadJevKey() { return {}; }
QString saveJevKey(const QString &) { return "このOSでの安全なキー保存は未対応です。今回はメモリ内で使用します"; }
}
#endif
