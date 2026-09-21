#pragma once
#include <QString>
namespace comments {
struct StoredKey { QString key; QString error; };
StoredKey loadJevKey();
QString saveJevKey(const QString &key);
}
