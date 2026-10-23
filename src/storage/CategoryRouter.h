#pragma once

#include "core/Types.h"
#include <QString>
#include <QStringList>

namespace idm::storage {

class CategoryRouter final {
public:
    CategoryRouter() = delete;

    [[nodiscard]] static core::FileCategory classifyFileName(const QString& fileName);
    [[nodiscard]] static QString categoryDisplayName(core::FileCategory category);
    [[nodiscard]] static QString defaultDirectoryForCategory(core::FileCategory category);
    [[nodiscard]] static QString defaultDirectoryForFile(const QString& fileName);
    [[nodiscard]] static QStringList defaultCapturedExtensions();
    [[nodiscard]] static bool matchesExtensionList(
        const QString& fileNameOrUrlPath,
        const QStringList& extensions
    );
};

} // namespace idm::storage
