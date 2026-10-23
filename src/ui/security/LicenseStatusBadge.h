#pragma once

#include "security/SecurityTypes.h"
#include <QString>
#include <QWidget>

namespace idm::ui::security {

class LicenseStatusBadge final : public QWidget {
    Q_OBJECT

public:
    explicit LicenseStatusBadge(QWidget* parent = nullptr);
    ~LicenseStatusBadge() override = default;

    void updateBadgeState(idm::security::LicenseTier tier, const QString& labelText);

    [[nodiscard]] QSize sizeHint() const override;

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    idm::security::LicenseTier m_tier{idm::security::LicenseTier::Trial};
    QString m_labelText{QStringLiteral("TRIAL: 14 DAYS")};
    bool m_hovered{false};
};

} // namespace idm::ui::security
