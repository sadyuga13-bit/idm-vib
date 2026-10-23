#pragma once

#include <QHash>
#include <QString>

namespace idm::i18n {

inline QHash<QString, QString> buildGlobalDictionary(const QString& lang) {
    if (lang == QStringLiteral("es")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("Añadir URL")},
            {QStringLiteral("tb.batch"), QStringLiteral("Lote")},
            {QStringLiteral("tb.resume"), QStringLiteral("Reanudar")},
            {QStringLiteral("tb.pause"), QStringLiteral("Pausar")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("Detener Todo")},
            {QStringLiteral("tb.delete"), QStringLiteral("Eliminar")},
            {QStringLiteral("tb.location"), QStringLiteral("Carpeta")},
            {QStringLiteral("tb.verify"), QStringLiteral("Verificar")},
            {QStringLiteral("tb.settings"), QStringLiteral("Ajustes")},
            {QStringLiteral("settings.title"), QStringLiteral("Opciones y Configuración del Sistema")},
            {QStringLiteral("settings.page.general"), QStringLiteral("General e Idioma")},
            {QStringLiteral("settings.page.filetypes"), QStringLiteral("Tipos de Archivo y Dominios")},
            {QStringLiteral("settings.page.paths"), QStringLiteral("Rutas de Guardado")},
            {QStringLiteral("settings.page.connection"), QStringLiteral("Conexiones y Límites")},
            {QStringLiteral("settings.page.proxy"), QStringLiteral("Proxy y Credenciales")},
            {QStringLiteral("settings.page.completion"), QStringLiteral("Ventanas, Antivirus y Sonidos")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("Guardar y Aplicar")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("Cancelar")}
        };
    }
    if (lang == QStringLiteral("fr")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("Ajouter URL")},
            {QStringLiteral("tb.batch"), QStringLiteral("Lot")},
            {QStringLiteral("tb.resume"), QStringLiteral("Reprendre")},
            {QStringLiteral("tb.pause"), QStringLiteral("Pause")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("Tout Arrêter")},
            {QStringLiteral("tb.delete"), QStringLiteral("Supprimer")},
            {QStringLiteral("tb.location"), QStringLiteral("Dossier")},
            {QStringLiteral("tb.verify"), QStringLiteral("Vérifier")},
            {QStringLiteral("tb.settings"), QStringLiteral("Paramètres")},
            {QStringLiteral("settings.title"), QStringLiteral("Options et Configuration Système")},
            {QStringLiteral("settings.page.general"), QStringLiteral("Général et Langue")},
            {QStringLiteral("settings.page.filetypes"), QStringLiteral("Types de Fichiers")},
            {QStringLiteral("settings.page.paths"), QStringLiteral("Dossiers d'Enregistrement")},
            {QStringLiteral("settings.page.connection"), QStringLiteral("Connexions et Serveurs")},
            {QStringLiteral("settings.page.proxy"), QStringLiteral("Proxy et Identifiants")},
            {QStringLiteral("settings.page.completion"), QStringLiteral("Antivirus et Sons")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("Enregistrer")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("Annuler")}
        };
    }
    if (lang == QStringLiteral("de")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("URL Hinzufügen")},
            {QStringLiteral("tb.batch"), QStringLiteral("Stapel")},
            {QStringLiteral("tb.resume"), QStringLiteral("Fortsetzen")},
            {QStringLiteral("tb.pause"), QStringLiteral("Pause")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("Alle Stoppen")},
            {QStringLiteral("tb.delete"), QStringLiteral("Löschen")},
            {QStringLiteral("tb.location"), QStringLiteral("Ordner")},
            {QStringLiteral("tb.verify"), QStringLiteral("Prüfen")},
            {QStringLiteral("tb.settings"), QStringLiteral("Einstellungen")},
            {QStringLiteral("settings.title"), QStringLiteral("System- und Programmoptionen")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("Speichern & Anwenden")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("Abbrechen")}
        };
    }
    if (lang == QStringLiteral("ru")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("Добавить URL")},
            {QStringLiteral("tb.batch"), QStringLiteral("Пакет")},
            {QStringLiteral("tb.resume"), QStringLiteral("Продолжить")},
            {QStringLiteral("tb.pause"), QStringLiteral("Пауза")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("Стоп все")},
            {QStringLiteral("tb.delete"), QStringLiteral("Удалить")},
            {QStringLiteral("tb.location"), QStringLiteral("Папка")},
            {QStringLiteral("tb.verify"), QStringLiteral("Проверка")},
            {QStringLiteral("tb.settings"), QStringLiteral("Настройки")},
            {QStringLiteral("settings.title"), QStringLiteral("Параметры и настройка системы")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("Сохранить")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("Отмена")}
        };
    }
    if (lang == QStringLiteral("zh_CN")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("新建下载")},
            {QStringLiteral("tb.batch"), QStringLiteral("批量下载")},
            {QStringLiteral("tb.resume"), QStringLiteral("继续")},
            {QStringLiteral("tb.pause"), QStringLiteral("暂停")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("全部停止")},
            {QStringLiteral("tb.delete"), QStringLiteral("删除")},
            {QStringLiteral("tb.location"), QStringLiteral("打开目录")},
            {QStringLiteral("tb.verify"), QStringLiteral("校验哈希")},
            {QStringLiteral("tb.settings"), QStringLiteral("选项设置")},
            {QStringLiteral("settings.title"), QStringLiteral("全局选项与系统配置")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("保存并应用")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("取消")}
        };
    }
    if (lang == QStringLiteral("ja")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("URL追加")},
            {QStringLiteral("tb.batch"), QStringLiteral("一括追加")},
            {QStringLiteral("tb.resume"), QStringLiteral("再開")},
            {QStringLiteral("tb.pause"), QStringLiteral("一時停止")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("全停止")},
            {QStringLiteral("tb.delete"), QStringLiteral("削除")},
            {QStringLiteral("tb.location"), QStringLiteral("保存先")},
            {QStringLiteral("tb.verify"), QStringLiteral("検証")},
            {QStringLiteral("tb.settings"), QStringLiteral("設定")},
            {QStringLiteral("settings.title"), QStringLiteral("システム設定とオプション")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("保存して適用")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("キャンセル")}
        };
    }
    if (lang == QStringLiteral("tr")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("URL Ekle")},
            {QStringLiteral("tb.batch"), QStringLiteral("Toplu İndir")},
            {QStringLiteral("tb.resume"), QStringLiteral("Devam Et")},
            {QStringLiteral("tb.pause"), QStringLiteral("Duraklat")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("Tümünü Durdur")},
            {QStringLiteral("tb.delete"), QStringLiteral("Sil")},
            {QStringLiteral("tb.location"), QStringLiteral("Klasör")},
            {QStringLiteral("tb.verify"), QStringLiteral("Doğrula")},
            {QStringLiteral("tb.settings"), QStringLiteral("Ayarlar")},
            {QStringLiteral("settings.title"), QStringLiteral("Seçenekler ve Sistem Yapılandırması")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("Kaydet ve Uygula")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("İptal")}
        };
    }
    if (lang == QStringLiteral("pt_BR")) {
        return {
            {QStringLiteral("tb.add_url"), QStringLiteral("Adicionar URL")},
            {QStringLiteral("tb.batch"), QStringLiteral("Lote")},
            {QStringLiteral("tb.resume"), QStringLiteral("Retomar")},
            {QStringLiteral("tb.pause"), QStringLiteral("Pausar")},
            {QStringLiteral("tb.stop_all"), QStringLiteral("Parar Tudo")},
            {QStringLiteral("tb.delete"), QStringLiteral("Excluir")},
            {QStringLiteral("tb.location"), QStringLiteral("Pasta")},
            {QStringLiteral("tb.verify"), QStringLiteral("Verificar")},
            {QStringLiteral("tb.settings"), QStringLiteral("Configurações")},
            {QStringLiteral("settings.title"), QStringLiteral("Opções e Configuração do Sistema")},
            {QStringLiteral("settings.btn.save"), QStringLiteral("Salvar e Aplicar")},
            {QStringLiteral("settings.btn.cancel"), QStringLiteral("Cancelar")}
        };
    }
    return {};
}

} // namespace idm::i18n
