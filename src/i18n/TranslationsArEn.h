#pragma once

#include <QHash>
#include <QString>

namespace idm::i18n {

inline QHash<QString, QString> buildEnglishDictionary() {
    return {
        {QStringLiteral("app.title"), QStringLiteral("IDM Vibe")},
        {QStringLiteral("tb.add_url"), QStringLiteral("Add URL")},
        {QStringLiteral("tb.batch"), QStringLiteral("Batch")},
        {QStringLiteral("tb.resume"), QStringLiteral("Resume")},
        {QStringLiteral("tb.pause"), QStringLiteral("Pause")},
        {QStringLiteral("tb.stop_all"), QStringLiteral("Stop All")},
        {QStringLiteral("tb.delete"), QStringLiteral("Delete")},
        {QStringLiteral("tb.location"), QStringLiteral("Folder")},
        {QStringLiteral("tb.verify"), QStringLiteral("Verify")},
        {QStringLiteral("tb.day_mode"), QStringLiteral("Light")},
        {QStringLiteral("tb.dark_mode"), QStringLiteral("Dark")},
        {QStringLiteral("tb.settings"), QStringLiteral("Options")},
        {QStringLiteral("tb.filter_placeholder"), QStringLiteral("Search...")},
        {QStringLiteral("settings.title"), QStringLiteral("Options")},
        {QStringLiteral("settings.page.general"), QStringLiteral("General")},
        {QStringLiteral("settings.page.filetypes"), QStringLiteral("File Types")},
        {QStringLiteral("settings.page.paths"), QStringLiteral("Save To")},
        {QStringLiteral("settings.page.connection"), QStringLiteral("Connection")},
        {QStringLiteral("settings.page.proxy"), QStringLiteral("Proxy / Socks")},
        {QStringLiteral("settings.page.completion"), QStringLiteral("Downloads")},
        {QStringLiteral("settings.btn.save"), QStringLiteral("OK")},
        {QStringLiteral("settings.btn.cancel"), QStringLiteral("Cancel")},
        {QStringLiteral("tray.show"), QStringLiteral("Show Main Window")},
        {QStringLiteral("tray.add_url"), QStringLiteral("Add New Download...")},
        {QStringLiteral("tray.resume_all"), QStringLiteral("Start Main Queue")},
        {QStringLiteral("tray.stop_all"), QStringLiteral("Stop All")},
        {QStringLiteral("tray.autostart"), QStringLiteral("Launch at Windows Startup")},
        {QStringLiteral("tray.exit"), QStringLiteral("Exit")},
        {QStringLiteral("prog.tab.status"), QStringLiteral("Download status")},
        {QStringLiteral("prog.tab.speed"), QStringLiteral("Speed Limiter")},
        {QStringLiteral("prog.tab.completion"), QStringLiteral("Options on completion")},
        {QStringLiteral("comp.title"), QStringLiteral("Download Complete")},
        {QStringLiteral("comp.open_file"), QStringLiteral("Open")},
        {QStringLiteral("comp.open_folder"), QStringLiteral("Open Folder")},
        {QStringLiteral("comp.close"), QStringLiteral("Close")}
    };
}

inline QHash<QString, QString> buildArabicDictionary() {
    return {
        {QStringLiteral("app.title"), QStringLiteral("IDM Vibe")},
        {QStringLiteral("tb.add_url"), QStringLiteral("إضافة رابط")},
        {QStringLiteral("tb.batch"), QStringLiteral("دفعة")},
        {QStringLiteral("tb.resume"), QStringLiteral("استئناف")},
        {QStringLiteral("tb.pause"), QStringLiteral("إيقاف")},
        {QStringLiteral("tb.stop_all"), QStringLiteral("إيقاف الكل")},
        {QStringLiteral("tb.delete"), QStringLiteral("حذف")},
        {QStringLiteral("tb.location"), QStringLiteral("المجلد")},
        {QStringLiteral("tb.verify"), QStringLiteral("فحص")},
        {QStringLiteral("tb.day_mode"), QStringLiteral("فاتح")},
        {QStringLiteral("tb.dark_mode"), QStringLiteral("داكن")},
        {QStringLiteral("tb.settings"), QStringLiteral("الخيارات")},
        {QStringLiteral("tb.filter_placeholder"), QStringLiteral("بحث...")},
        {QStringLiteral("settings.title"), QStringLiteral("الخيارات")},
        {QStringLiteral("settings.page.general"), QStringLiteral("عام")},
        {QStringLiteral("settings.page.filetypes"), QStringLiteral("أنواع الملفات")},
        {QStringLiteral("settings.page.paths"), QStringLiteral("الحفظ")},
        {QStringLiteral("settings.page.connection"), QStringLiteral("الاتصال")},
        {QStringLiteral("settings.page.proxy"), QStringLiteral("الوكيل والمواقع")},
        {QStringLiteral("settings.page.completion"), QStringLiteral("التنزيلات")},
        {QStringLiteral("settings.btn.save"), QStringLiteral("حفظ")},
        {QStringLiteral("settings.btn.cancel"), QStringLiteral("إلغاء")},
        {QStringLiteral("tray.show"), QStringLiteral("إظهار النافذة الرئيسية")},
        {QStringLiteral("tray.add_url"), QStringLiteral("إضافة رابط جديد...")},
        {QStringLiteral("tray.resume_all"), QStringLiteral("بدء الطابور الرئيسي")},
        {QStringLiteral("tray.stop_all"), QStringLiteral("إيقاف الكل")},
        {QStringLiteral("tray.autostart"), QStringLiteral("التشغيل مع بدء ويندوز")},
        {QStringLiteral("tray.exit"), QStringLiteral("خروج")},
        {QStringLiteral("prog.tab.status"), QStringLiteral("حالة التحميل")},
        {QStringLiteral("prog.tab.speed"), QStringLiteral("محدد السرعة")},
        {QStringLiteral("prog.tab.completion"), QStringLiteral("خيارات عند الاكتمال")},
        {QStringLiteral("comp.title"), QStringLiteral("اكتمل التحميل")},
        {QStringLiteral("comp.open_file"), QStringLiteral("فتح")},
        {QStringLiteral("comp.open_folder"), QStringLiteral("فتح المجلد")},
        {QStringLiteral("comp.close"), QStringLiteral("إغلاق")}
    };
}

} // namespace idm::i18n
