#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QSizePolicy>
#include <QTimer>
#include <QDateTime>
#include <QFont>
#include <QPalette>
#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMap>
#include <QStringList>
#include <QList>
#include <QPointer>
#include <QPushButton>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QIntValidator>

// =========================================================
// CONFIG
// =========================================================

struct Config {
    QString api_key;
    double lat;
    double lon;
    int method;

    static Config load() {
        Config cfg;
        cfg.lat = 0.0;
        cfg.lon = 0.0;
        cfg.method = 4;
        cfg.api_key = "";

        QFile file("config.json");
        if (!file.open(QIODevice::ReadOnly)) {
            // Create default config file
            cfg.save();
            return cfg;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) return cfg;

        QJsonObject obj = doc.object();
        cfg.api_key = obj["api_key"].toString();
        QJsonObject loc = obj["location"].toObject();
        cfg.lat = loc["lat"].toDouble(0.0);
        cfg.lon = loc["lon"].toDouble(0.0);
        cfg.method = obj["method"].toInt(4);
        return cfg;
    }

    bool save() const {
        QJsonObject root;
        root["api_key"] = api_key;
        QJsonObject loc;
        loc["lat"] = lat;
        loc["lon"] = lon;
        loc["name"] = "User Location";
        root["location"] = loc;
        root["method"] = method;

        QFile file("config.json");
        if (!file.open(QIODevice::WriteOnly))
            return false;
        QJsonDocument doc(root);
        file.write(doc.toJson());
        file.close();
        return true;
    }

    QString buildApiUrl() const {
        return QString("https://islamicapi.com/api/v1/prayer-time/?lat=%1&lon=%2&method=%3&api_key=%4")
            .arg(lat, 0, 'f', 6)
            .arg(lon, 0, 'f', 6)
            .arg(method)
            .arg(api_key);
    }
};

// =========================================================
// CONSTANTS
// =========================================================

const QString CACHE_FILE = "prayer_cache.json";
const QStringList PRAYER_ORDER = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha", "Firstthird", "Midnight", "Lastthird"};

// =========================================================
// THEME (unchanged)
// =========================================================

struct Theme {
    static QString bg() { return "#002b36"; }
    static QString blue() { return "#268bd2"; }
    static QString cyan() { return "#2aa198"; }
    static QString green() { return "#859900"; }
    static QString magenta() { return "#d33682"; }
    static QString muted() { return "#93a1a1"; }
    static QString orange() { return "#cb4b16"; }
    static QString red() { return "#dc322f"; }
    static QString text() { return "#839496"; }
    static QString violet() { return "#6c71c4"; }
    static QString yellow() { return "#b58900"; }
};

// =========================================================
// UTILITIES (unchanged)
// =========================================================

int toMinutes(const QString& timeStr) {
    QStringList parts = timeStr.split(":");
    if (parts.size() != 2) return 0;
    int h = parts[0].toInt();
    int m = parts[1].toInt();
    return h * 60 + m;
}

QString to12(const QString& timeStr) {
    QStringList parts = timeStr.split(":");
    if (parts.size() != 2) return timeStr;
    int h = parts[0].toInt();
    int m = parts[1].toInt();
    QString suffix = "AM";
    if (h == 0) {
        h = 12;
    } else if (h == 12) {
        suffix = "PM";
    } else if (h > 12) {
        h -= 12;
        suffix = "PM";
    }
    return QString("%1:%2 %3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(suffix);
}

// =========================================================
// CACHE (24h) - unchanged
// =========================================================

bool loadCache(QJsonObject& outData) {
    QFile file(CACHE_FILE);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull())
        return false;

    QJsonObject obj = doc.object();
    qint64 timestamp = obj["timestamp"].toInteger(0);
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (now - timestamp >= 86400) // 24h
        return false;

    outData = obj["data"].toObject();
    return true;
}

void saveCache(const QJsonObject& data) {
    QJsonObject root;
    root["timestamp"] = QDateTime::currentSecsSinceEpoch();
    root["data"] = data;

    QFile file(CACHE_FILE);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}

// =========================================================
// LOGIC (unchanged)
// =========================================================

QString computeCurrent(const QMap<QString, QString>& times) {
    QDateTime now = QDateTime::currentDateTime();
    int cur = now.time().hour() * 60 + now.time().minute();

    QMap<QString, int> mins;
    for (auto it = times.begin(); it != times.end(); ++it) {
        if (it.value().contains(":")) {
            mins[it.key()] = toMinutes(it.value());
        }
    }

    for (int i = 0; i < PRAYER_ORDER.size(); ++i) {
        QString name = PRAYER_ORDER[i];
        if (!mins.contains(name))
            continue;

        int start = mins[name];
        int end = (i < PRAYER_ORDER.size() - 1 && mins.contains(PRAYER_ORDER[i+1])) ? mins[PRAYER_ORDER[i+1]] : 24 * 60;

        if (start <= cur && cur < end)
            return name;
    }

    return "Fajr";
}

// =========================================================
// SETTINGS DIALOG
// =========================================================

class ConfigDialog : public QDialog {
    Q_OBJECT
public:
    ConfigDialog(const Config& current, QWidget* parent = nullptr)
        : QDialog(parent), config(current) {
        setWindowTitle("Settings");
        setMinimumWidth(350);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        QFormLayout* form = new QFormLayout();

        // API Key
        apiKeyEdit = new QLineEdit(config.api_key);
        apiKeyEdit->setEchoMode(QLineEdit::Password);
        form->addRow("API Key:", apiKeyEdit);

        // Latitude
        latEdit = new QDoubleSpinBox();
        latEdit->setRange(-90.0, 90.0);
        latEdit->setDecimals(6);
        latEdit->setValue(config.lat);
        form->addRow("Latitude:", latEdit);

        // Longitude
        lonEdit = new QDoubleSpinBox();
        lonEdit->setRange(-180.0, 180.0);
        lonEdit->setDecimals(6);
        lonEdit->setValue(config.lon);
        form->addRow("Longitude:", lonEdit);

        // Method
        methodCombo = new QComboBox();
        methodCombo->addItem("University of Islamic Sciences, Karachi", 1);
        methodCombo->addItem("Muslim World League (MWL)", 3);
        methodCombo->addItem("Umm al-Qura University, Makkah", 4);
        methodCombo->addItem("Egyptian General Authority of Survey", 5);
        // Find index of current method
        int idx = methodCombo->findData(config.method);
        if (idx >= 0) methodCombo->setCurrentIndex(idx);
        form->addRow("Method:", methodCombo);

        mainLayout->addLayout(form);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okBtn = new QPushButton("OK");
        QPushButton* cancelBtn = new QPushButton("Cancel");
        connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addStretch();
        buttonLayout->addWidget(okBtn);
        buttonLayout->addWidget(cancelBtn);
        mainLayout->addLayout(buttonLayout);
    }

    Config getConfig() const {
        Config newCfg;
        newCfg.api_key = apiKeyEdit->text().trimmed();
        newCfg.lat = latEdit->value();
        newCfg.lon = lonEdit->value();
        newCfg.method = methodCombo->currentData().toInt();
        return newCfg;
    }

private:
    Config config;
    QLineEdit* apiKeyEdit;
    QDoubleSpinBox* latEdit;
    QDoubleSpinBox* lonEdit;
    QComboBox* methodCombo;
};

// =========================================================
// MAIN WINDOW
// =========================================================

class PrayerTimesWindow : public QMainWindow {
    Q_OBJECT

public:
    PrayerTimesWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Prayer Times Dashboard");
        setMinimumSize(1000, 800);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        setStyleSheet(QString(
            "QMainWindow { background-color: %1; }"
            "QLabel { background-color: transparent; color: %2; }"
        ).arg(Theme::bg(), Theme::text()));

        setupUI();
        setupClock();

        // Load config and fetch
        refreshData();
    }

private slots:
    void updateClock() {
        if (!stateData.isEmpty()) {
            QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss ap");
            clockLabel->setText(currentTime);
        }

        if (!stateTimes.isEmpty()) {
            QString newCurrent = computeCurrent(stateTimes);
            if (newCurrent != stateCurrent) {
                stateCurrent = newCurrent;
                updatePrayerHighlight();
            }
        }
    }

    void onNetworkReply(QNetworkReply *reply) {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Network error:" << reply->errorString();
            QMessageBox::warning(this, "Network Error",
                                 "Failed to fetch prayer times.\nPlease check your API key and internet connection.");
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) {
            qDebug() << "Invalid JSON";
            return;
        }

        QJsonObject root = doc.object();
        if (!root.contains("data")) {
            QMessageBox::warning(this, "API Error",
                                 "Invalid response from API.\nPlease check your API key.");
            return;
        }

        QJsonObject dataObj = root["data"].toObject();
        saveCache(dataObj);
        processData(dataObj);
    }

    void openSettings() {
        ConfigDialog dialog(currentConfig, this);
        if (dialog.exec() == QDialog::Accepted) {
            Config newCfg = dialog.getConfig();
            if (newCfg.api_key.isEmpty()) {
                QMessageBox::warning(this, "Warning", "API key is empty. Please enter a valid key.");
                return;
            }
            currentConfig = newCfg;
            currentConfig.save();
            // Clear cache and refetch
            QFile::remove(CACHE_FILE);
            refreshData();
        }
    }

private:
    // UI components
    QLabel *timezoneLabel;
    QLabel *hijriLabel;
    QLabel *dateLabel;
    QLabel *clockLabel;
    QMap<QString, QLabel*> prayerLabels;
    QMap<QString, QLabel*> prayerTimeLabels;
    QWidget *prohibitedContainer;
    QVBoxLayout *prohibitedLayout;
    QPushButton *settingsButton;

    // State
    QJsonObject stateData;
    QMap<QString, QString> stateTimes;
    QMap<QString, QMap<QString, QString>> stateProhibited;
    QString stateCurrent;
    Config currentConfig;

    // Network
    QNetworkAccessManager *networkManager;

    void setupUI() {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(30, 30, 30, 30);

        // Top section with settings button
        QWidget *topWidget = new QWidget();
        QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
        topLayout->setAlignment(Qt::AlignCenter);
        topLayout->setSpacing(30);

        // Left spacer
        topLayout->addStretch();

        // Info labels (vertically stacked)
        QWidget *infoWidget = new QWidget();
        QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setAlignment(Qt::AlignCenter);
        infoLayout->setSpacing(8);

        timezoneLabel = new QLabel();
        timezoneLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::orange()));
        timezoneLabel->setAlignment(Qt::AlignCenter);
        QFont tzFont("Helvetica", 16, QFont::Bold);
        timezoneLabel->setFont(tzFont);
        infoLayout->addWidget(timezoneLabel);

        hijriLabel = new QLabel();
        hijriLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::cyan()));
        hijriLabel->setAlignment(Qt::AlignCenter);
        QFont hijriFont("Helvetica", 18, QFont::Bold);
        hijriLabel->setFont(hijriFont);
        infoLayout->addWidget(hijriLabel);

        dateLabel = new QLabel();
        dateLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::violet()));
        dateLabel->setAlignment(Qt::AlignCenter);
        QFont dateFont("Helvetica", 20, QFont::Bold);
        dateLabel->setFont(dateFont);
        infoLayout->addWidget(dateLabel);

        clockLabel = new QLabel();
        clockLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::yellow()));
        clockLabel->setAlignment(Qt::AlignCenter);
        QFont clockFont("Helvetica", 24, QFont::Bold);
        clockLabel->setFont(clockFont);
        infoLayout->addWidget(clockLabel);

        topLayout->addWidget(infoWidget);

        // Right spacer and settings button
        topLayout->addStretch();
        settingsButton = new QPushButton("⚙ Settings");
        settingsButton->setStyleSheet(QString("color: %1; background-color: #073642; border: 1px solid %2; padding: 8px 16px; border-radius: 4px;")
                                      .arg(Theme::blue(), Theme::blue()));
        QFont btnFont("Helvetica", 12, QFont::Bold);
        settingsButton->setFont(btnFont);
        connect(settingsButton, &QPushButton::clicked, this, &PrayerTimesWindow::openSettings);
        topLayout->addWidget(settingsButton);

        mainLayout->addWidget(topWidget);

        // Main content
        QWidget *contentWidget = new QWidget();
        QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
        contentLayout->setSpacing(50);
        contentLayout->setContentsMargins(20, 20, 20, 20);

        // Left section - Prayer Times
        QWidget *leftWidget = new QWidget();
        QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setAlignment(Qt::AlignTop);
        leftLayout->setSpacing(12);

        QLabel *titleLeft = new QLabel("Prayer Times");
        titleLeft->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::green()));
        titleLeft->setAlignment(Qt::AlignCenter);
        QFont titleFont("Helvetica", 26, QFont::Bold);
        titleLeft->setFont(titleFont);
        leftLayout->addWidget(titleLeft);
        leftLayout->addSpacing(15);

        QFont prayerFont("Helvetica", 18, QFont::Bold);
        QFont timeFont("Helvetica", 18);

        for (const QString& name : PRAYER_ORDER) {
            QWidget *rowWidget = new QWidget();
            QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(10, 5, 10, 5);
            rowLayout->setSpacing(20);

            QLabel *lbl = new QLabel(name);
            lbl->setFont(prayerFont);
            lbl->setStyleSheet(QString("color: %1;").arg(Theme::text()));
            lbl->setFixedWidth(140);
            rowLayout->addWidget(lbl);

            QLabel *val = new QLabel("--:--");
            val->setFont(timeFont);
            val->setStyleSheet(QString("color: %1;").arg(Theme::text()));
            val->setAlignment(Qt::AlignRight);
            rowLayout->addWidget(val);

            leftLayout->addWidget(rowWidget);
            prayerLabels[name] = lbl;
            prayerTimeLabels[name] = val;
        }

        contentLayout->addWidget(leftWidget);

        // Right section - Prohibited Times
        QWidget *rightWidget = new QWidget();
        QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setAlignment(Qt::AlignTop);
        rightLayout->setSpacing(12);

        QLabel *titleRight = new QLabel("Prohibited Times");
        titleRight->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::red()));
        titleRight->setAlignment(Qt::AlignCenter);
        titleRight->setFont(titleFont);
        rightLayout->addWidget(titleRight);
        rightLayout->addSpacing(15);

        prohibitedContainer = new QWidget();
        prohibitedLayout = new QVBoxLayout(prohibitedContainer);
        prohibitedLayout->setAlignment(Qt::AlignTop);
        prohibitedLayout->setSpacing(8);
        rightLayout->addWidget(prohibitedContainer);

        contentLayout->addWidget(rightWidget);
        mainLayout->addWidget(contentWidget);

        contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setupClock() {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &PrayerTimesWindow::updateClock);
        timer->start(1000);
    }

    void refreshData() {
        currentConfig = Config::load();
        if (currentConfig.api_key.isEmpty()) {
            QMessageBox::warning(this, "API Key Missing",
                                 "Please set your API key in Settings.\nYou can get one from https://islamicapi.com");
            // Show empty state
            return;
        }

        // Try cache
        QJsonObject cached;
        if (loadCache(cached)) {
            processData(cached);
            return;
        }

        // Otherwise fetch
        networkManager = new QNetworkAccessManager(this);
        connect(networkManager, &QNetworkAccessManager::finished, this, &PrayerTimesWindow::onNetworkReply);

        QUrl url(currentConfig.buildApiUrl());
        QNetworkRequest request(url);
        networkManager->get(request);
    }

    void processData(const QJsonObject& data) {
        stateData = data;

        // Parse times
        QJsonObject timesObj = data["times"].toObject();
        stateTimes.clear();
        for (auto it = timesObj.begin(); it != timesObj.end(); ++it) {
            stateTimes[it.key()] = it.value().toString();
        }

        // Parse prohibited times
        QJsonObject prohibitedObj = data["prohibited_times"].toObject();
        stateProhibited.clear();
        for (auto it = prohibitedObj.begin(); it != prohibitedObj.end(); ++it) {
            QJsonObject blockObj = it.value().toObject();
            QMap<QString, QString> inner;
            for (auto it2 = blockObj.begin(); it2 != blockObj.end(); ++it2) {
                inner[it2.key()] = it2.value().toString();
            }
            stateProhibited[it.key()] = inner;
        }

        stateCurrent = computeCurrent(stateTimes);

        updateUI();
    }

    void updateUI() {
        if (stateData.isEmpty())
            return;

        QJsonObject dateObj = stateData["date"].toObject();
        QJsonObject greg = dateObj["gregorian"].toObject();
        QJsonObject hijri = dateObj["hijri"].toObject();

        QString dateStr = QString("%1, %2 %3 %4")
                              .arg(greg["weekday"].toObject()["en"].toString())
                              .arg(greg["day"].toString())
                              .arg(greg["month"].toObject()["en"].toString())
                              .arg(greg["year"].toString());
        dateLabel->setText(dateStr);

        int monthNumber = hijri["month"].toObject()["number"].toInt();
        QString hijriStr = QString("%1 %2 (%3) %4")
                            .arg(hijri["day"].toString())
                            .arg(hijri["month"].toObject()["en"].toString())
                            .arg(monthNumber)
                            .arg(hijri["year"].toString());

        hijriLabel->setText(hijriStr);

        QJsonObject tz = stateData["timezone"].toObject();
        QString tzName = tz["name"].toString().replace("\\/", "/");
        QString tzOffset = tz["utc_offset"].toString();
        timezoneLabel->setText(QString("%1 (UTC%2)").arg(tzName).arg(tzOffset));

        // Update prayer times and highlighting
        updatePrayerHighlight();

        // Update prohibited times
        // Clear existing
        QLayoutItem *child;
        while ((child = prohibitedLayout->takeAt(0)) != nullptr) {
            if (child->widget())
                delete child->widget();
            delete child;
        }

        QFont blockFont("Helvetica", 14, QFont::Bold);
        QFont keyFont("Helvetica", 16);
        QFont valueFont("Helvetica", 17, QFont::Bold);

        for (auto it = stateProhibited.begin(); it != stateProhibited.end(); ++it) {
            QString blockName = it.key();
            QLabel *blockLabel = new QLabel(blockName.toUpper());
            blockLabel->setFont(blockFont);
            blockLabel->setStyleSheet(QString("color: %1;").arg(Theme::blue()));
            blockLabel->setContentsMargins(0, 15, 0, 5);
            prohibitedLayout->addWidget(blockLabel);

            QMap<QString, QString> inner = it.value();
            for (auto it2 = inner.begin(); it2 != inner.end(); ++it2) {
                QWidget *rowWidget = new QWidget();
                QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
                rowLayout->setContentsMargins(10, 3, 10, 3);
                rowLayout->setSpacing(15);

                QLabel *keyLabel = new QLabel(it2.key());
                keyLabel->setFont(keyFont);
                keyLabel->setStyleSheet(QString("color: %1;").arg(Theme::text()));
                rowLayout->addWidget(keyLabel);

                QLabel *valueLabel = new QLabel(to12(it2.value()));
                valueLabel->setFont(valueFont);
                valueLabel->setStyleSheet(QString("color: %1;").arg(Theme::text()));
                valueLabel->setAlignment(Qt::AlignRight);
                rowLayout->addWidget(valueLabel);

                prohibitedLayout->addWidget(rowWidget);
            }
        }
    }

    void updatePrayerHighlight() {
        QFont prayerFont("Helvetica", 18, QFont::Bold);
        QFont timeFont("Helvetica", 18);

        for (const QString& name : PRAYER_ORDER) {
            if (!stateTimes.contains(name))
                continue;

            QString color = (name == stateCurrent) ? Theme::blue() : Theme::text();
            prayerLabels[name]->setStyleSheet(QString("color: %1;").arg(color));
            prayerLabels[name]->setFont(prayerFont);

            QString timeStr = stateTimes[name];
            prayerTimeLabels[name]->setText(to12(timeStr));
            prayerTimeLabels[name]->setStyleSheet(QString("color: %1;").arg(color));
            prayerTimeLabels[name]->setFont(timeFont);
        }
    }
};

// =========================================================
// MAIN
// =========================================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    PrayerTimesWindow window;
    window.show();
    window.setGeometry(100, 100, 1200, 900);
    return app.exec();
}

#include "main.moc"