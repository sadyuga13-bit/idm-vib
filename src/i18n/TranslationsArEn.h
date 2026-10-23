#pragma once

#include <QHash>
#include <QString>

namespace idm::i18n {

inline QHash<QString, QString> buildEnglishDictionary() {
    return {
        {QStringLiteral("app.title"), QStringLiteral("IDM Vibe - Native C++20 Multi-Segment & YouTube Download Engine")},
        {QStringLiteral("tb.add_url"), QStringLiteral("Add URL")},
        {QStringLiteral("tb.batch"), QStringLiteral("Batch")},
        {QStringLiteral("tb.resume"), QStringLiteral("Resume")},
        {QStringLiteral("tb.pause"), QStringLiteral("Pause")},
        {QStringLiteral("tb.stop_all"), QStringLiteral("Stop All")},
        {QStringLiteral("tb.delete"), QStringLiteral("Delete")},
        {QStringLiteral("tb.location"), QStringLiteral("Location")},
        {QStringLiteral("tb.verify"), QStringLiteral("Verify")},
        {QStringLiteral("tb.day_mode"), QStringLiteral("Day")},
        {QStringLiteral("tb.dark_mode"), QStringLiteral("Dark")},
        {QStringLiteral("tb.settings"), QStringLiteral("Settings")},
        {QStringLiteral("tb.filter_placeholder"), QStringLiteral("Filter tasks...")},
        {QStringLiteral("settings.title"), QStringLiteral("Options & System Configuration")},
        {QStringLiteral("settings.page.general"), QStringLiteral("General & Language")},
        {QStringLiteral("settings.page.filetypes"), QStringLiteral("File Types & Domains")},
        {QStringLiteral("settings.page.paths"), QStringLiteral("Save Paths & .part")},
        {QStringLiteral("settings.page.connection"), QStringLiteral("Connections & Limits")},
        {QStringLiteral("settings.page.proxy"), QStringLiteral("Proxy & Site Logins")},
        {QStringLiteral("settings.page.completion"), QStringLiteral("Popups, AV & Sounds")},
        {QStringLiteral("settings.btn.save"), QStringLiteral("Save & Apply")},
        {QStringLiteral("settings.btn.cancel"), QStringLiteral("Cancel")},
        {QStringLiteral("tray.show"), QStringLiteral("Show Main Window")},
        {QStringLiteral("tray.add_url"), QStringLiteral("Add New Download...")},
        {QStringLiteral("tray.resume_all"), QStringLiteral("Start / Resume Main Queue")},
        {QStringLiteral("tray.stop_all"), QStringLiteral("Stop All Active Downloads")},
        {QStringLiteral("tray.autostart"), QStringLiteral("Launch at Windows Startup")},
        {QStringLiteral("tray.exit"), QStringLiteral("Exit Completely")},
        {QStringLiteral("prog.tab.status"), QStringLiteral("Download Status")},
        {QStringLiteral("prog.tab.speed"), QStringLiteral("Speed Limiter")},
        {QStringLiteral("prog.tab.completion"), QStringLiteral("Options on Completion")},
        {QStringLiteral("comp.title"), QStringLiteral("Download Complete")},
        {QStringLiteral("comp.open_file"), QStringLiteral("Open File")},
        {QStringLiteral("comp.open_folder"), QStringLiteral("Open Folder")},
        {QStringLiteral("comp.close"), QStringLiteral("Close")}
    };
}

inline QHash<QString, QString> buildArabicDictionary() {
    return {
        {QStringLiteral("app.title"), QStringLiteral("IDM Vibe - محرك التحميل متعدد المسارات ويوتيوب (C++20)")},
        {QStringLiteral("tb.add_url"), QStringLiteral("إضافة رابط")},
        {QStringLiteral("tb.batch"), QStringLiteral("دفعة روابط")},
        {QStringLiteral("tb.resume"), QStringLiteral("استئناف")},
        {QStringLiteral("tb.pause"), QStringLiteral("إيقاف مؤقت")},
        {QStringLiteral("tb.stop_all"), QStringLiteral("إيقاف الكل")},
        {QStringLiteral("tb.delete"), QStringLiteral("حذف")},
        {QStringLiteral("tb.location"), QStringLiteral("المجلد")},
        {QStringLiteral("tb.verify"), QStringLiteral("فحص البصمة")},
        {QStringLiteral("tb.day_mode"), QStringLiteral("نهاري")},
        {QStringLiteral("tb.dark_mode"), QStringLiteral("ليلي")},
        {QStringLiteral("tb.settings"), QStringLiteral("الإعدادات")},
        {QStringLiteral("tb.filter_placeholder"), QStringLiteral("بحث في التحميلات...")},
        {QStringLiteral("settings.title"), QStringLiteral("الإعدادات الشاملة وتكوين النظام")},
        {QStringLiteral("settings.page.general"), QStringLiteral("العام واللغات")},
        {QStringLiteral("settings.page.filetypes"), QStringLiteral("أنواع الملفات والمواقع")},
        {QStringLiteral("settings.page.paths"), QStringLiteral("مسارات الحفظ والمؤقت")},
        {QStringLiteral("settings.page.connection"), QStringLiteral("الاتصالات والسيرفرات")},
        {QStringLiteral("settings.page.proxy"), QStringLiteral("البروكسي وخزنة المواقع")},
        {QStringLiteral("settings.page.completion"), QStringLiteral("النوافذ وفحص الحماية")},
        {QStringLiteral("settings.btn.save"), QStringLiteral("حفظ وتطبيق الإعدادات")},
        {QStringLiteral("settings.btn.cancel"), QStringLiteral("إلغاء الأمر")},
        {QStringLiteral("tray.show"), QStringLiteral("إظهار البرنامج الرئيسي")},
        {QStringLiteral("tray.add_url"), QStringLiteral("إضافة رابط تحميل جديد...")},
        {QStringLiteral("tray.resume_all"), QStringLiteral("بدء / استئناف الطابور الرئيسي")},
        {QStringLiteral("tray.stop_all"), QStringLiteral("إيقاف جميع التحميلات النشطة")},
        {QStringLiteral("tray.autostart"), QStringLiteral("التشغيل التلقائي مع بدء ويندوز")},
        {QStringLiteral("tray.exit"), QStringLiteral("إنهاء البرنامج بالكامل")},
        {QStringLiteral("prog.tab.status"), QStringLiteral("حالة التحميل الحية")},
        {QStringLiteral("prog.tab.speed"), QStringLiteral("محدد السرعة للملف")},
        {QStringLiteral("prog.tab.completion"), QStringLiteral("خيارات عند الاكتمال")},
        {QStringLiteral("comp.title"), QStringLiteral("اكتمل التحميل بنجاح")},
        {QStringLiteral("comp.open_file"), QStringLiteral("فتح الملف")},
        {QStringLiteral("comp.open_folder"), QStringLiteral("فتح المجلد")},
        {QStringLiteral("comp.close"), QStringLiteral("إغلاق")}
    };
}

} // namespace idm::i18n
