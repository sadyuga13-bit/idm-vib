#include "ui/security/SeatManagerWidget.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSysInfo>
#include <QVBoxLayout>

namespace idm::ui::security {

using idm::security::HwidComponents;
using idm::security::SeatInfo;
using theme::IconId;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

SeatManagerWidget::SeatManagerWidget(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 6, 0, 0);
    root->setSpacing(8);

    auto* topBar = new QHBoxLayout();
    auto* devIcon = new QLabel(this);
    devIcon->setPixmap(IconRegistry::pixmap(IconId::DeviceLaptop, 16, IconState::Normal));
    m_lblUsageSummary = new QLabel(QStringLiteral("ACTIVE HARDWARE SEATS: 1 / 3 USED"), this);
    m_lblUsageSummary->setFont(ThemeTokens::monoFont(true));
    m_lblUsageSummary->setStyleSheet(QStringLiteral("color: #E6E8EC;"));

    m_btnRevoke = new QPushButton(QStringLiteral("Deactivate Selected Seat"), this);
    m_btnRevoke->setIcon(IconRegistry::icon(IconId::TrashDelete, 14));
    m_btnRevoke->setEnabled(false);

    topBar->addWidget(devIcon);
    topBar->addWidget(m_lblUsageSummary, 1);
    topBar->addWidget(m_btnRevoke);
    root->addLayout(topBar);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Device Hostname"),
        QStringLiteral("HWID Quorum Prefix"),
        QStringLiteral("Platform"),
        QStringLiteral("Bound Timestamp"),
        QStringLiteral("Seat State")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);
    m_table->horizontalHeader()->setFixedHeight(26);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 145);
    m_table->setColumnWidth(1, 155);
    m_table->setColumnWidth(2, 125);
    m_table->setColumnWidth(3, 130);
    root->addWidget(m_table, 1);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_table->currentRow();
        m_btnRevoke->setEnabled(row >= 0 && row < m_seats.size());
    });
    connect(m_btnRevoke, &QPushButton::clicked, this, &SeatManagerWidget::onRevokeClicked);
}

void SeatManagerWidget::setSeats(const QList<SeatInfo>& seats, int maxSeats) {
    m_seats = seats;
    m_lblUsageSummary->setText(
        QStringLiteral("ACTIVE HARDWARE SEATS: %1 / %2 USED")
            .arg(m_seats.size())
            .arg(maxSeats)
    );

    m_table->setRowCount(static_cast<int>(m_seats.size()));
    for (int i = 0; i < m_seats.size(); ++i) {
        const SeatInfo& s = m_seats.at(i);

        auto* itemHost = new QTableWidgetItem(s.deviceName);
        itemHost->setIcon(IconRegistry::icon(IconId::DeviceLaptop, 14));

        auto* itemHwid = new QTableWidgetItem(s.hwidPrefix);
        itemHwid->setFont(ThemeTokens::monoFont(false));

        auto* itemOs = new QTableWidgetItem(s.osName);
        auto* itemDate = new QTableWidgetItem(s.activatedAtIso);
        itemDate->setFont(ThemeTokens::monoFont(false));

        auto* itemState = new QTableWidgetItem(
            s.isCurrentDevice ? QStringLiteral("THIS WORKSTATION") : QStringLiteral("REMOTE SEAT")
        );
        itemState->setFont(ThemeTokens::monoFont(true));
        itemState->setForeground(
            s.isCurrentDevice ? ThemeTokens::statusDone() : ThemeTokens::statusActive()
        );

        m_table->setItem(i, 0, itemHost);
        m_table->setItem(i, 1, itemHwid);
        m_table->setItem(i, 2, itemOs);
        m_table->setItem(i, 3, itemDate);
        m_table->setItem(i, 4, itemState);
    }
    m_btnRevoke->setEnabled(false);
}

void SeatManagerWidget::populateDefaultLocalSeat(const HwidComponents& localHwid) {
    SeatInfo currentSeat;
    currentSeat.seatId = QStringLiteral("SEAT-LOCAL-01");
    currentSeat.deviceName = QSysInfo::machineHostName();
    currentSeat.hwidPrefix = localHwid.compositeFingerprint().left(17);
    currentSeat.osName = QSysInfo::prettyProductName();
    currentSeat.activatedAtIso = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    currentSeat.isCurrentDevice = true;
    setSeats({currentSeat}, 3);
}

void SeatManagerWidget::onRevokeClicked() {
    const int row = m_table->currentRow();
    if (row >= 0 && row < m_seats.size()) {
        emit revokeSeatRequested(m_seats.at(row).seatId);
    }
}

} // namespace idm::ui::security
