#pragma once

#include "security/SecurityTypes.h"
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace idm::ui::security {

class SeatManagerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SeatManagerWidget(QWidget* parent = nullptr);
    ~SeatManagerWidget() override = default;

    void setSeats(const QList<idm::security::SeatInfo>& seats, int maxSeats);
    void populateDefaultLocalSeat(const idm::security::HwidComponents& localHwid);

signals:
    void revokeSeatRequested(const QString& seatId);

private:
    void onRevokeClicked();

    QLabel* m_lblUsageSummary{nullptr};
    QTableWidget* m_table{nullptr};
    QPushButton* m_btnRevoke{nullptr};
    QList<idm::security::SeatInfo> m_seats;
};

} // namespace idm::ui::security
