/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <f1x/openauto/autoapp/UI/MainWindow.hpp>
#include <f1x/openauto/autoapp/UI/SettingsWindow.hpp>

#include <QFileInfo>
#include <QFile>
#include "ui_mainwindow.h"
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDatabase>
#include <QFont>
#include <QScreen>
#include <QRect>
#include <QVideoWidget>
#include <QNetworkInterface>
#include <QStandardItemModel>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <f1x/openauto/Common/Log.hpp>
#include <QDebug>
#include <QMetaType>
#include <sstream>

namespace f1x
{
	namespace openauto
	{
		namespace autoapp
		{
			namespace ui
			{

				MainWindow::MainWindow(configuration::IConfiguration::Pointer configuration, QWidget* parent)
					: QMainWindow(parent)
					, ui_(new Ui::MainWindow)
					, localDevice(new QBluetoothLocalDevice)
				{
					// set default bg color to black
					this->setStyleSheet("QMainWindow {background-color: rgb(0,0,0);}");

					// Set default font and size
					int id = QFontDatabase::addApplicationFont(":/Roboto-Regular.ttf");
					QString family = QFontDatabase::applicationFontFamilies(id).at(0);
					QFont _font(family, 11);
					qApp->setFont(_font);

					this->configuration_ = configuration;

					// trigger files
					this->nightModeEnabled = check_file_exist(this->nightModeFile);
					this->devModeEnabled = check_file_exist(this->devModeFile);
					this->wifiButtonForce = check_file_exist(this->wifiButtonFile);
					this->cameraButtonForce = check_file_exist(this->cameraButtonFile);
					this->brightnessButtonForce = check_file_exist(this->brightnessButtonFile);
					this->systemDebugmode = check_file_exist(this->debugModeFile);
					this->lightsensor = check_file_exist(this->lsFile);
					this->c1ButtonForce = check_file_exist(this->custom_button_file_c1);
					this->c2ButtonForce = check_file_exist(this->custom_button_file_c2);
					this->c3ButtonForce = check_file_exist(this->custom_button_file_c3);
					this->c4ButtonForce = check_file_exist(this->custom_button_file_c4);
					this->c5ButtonForce = check_file_exist(this->custom_button_file_c5);
					this->c6ButtonForce = check_file_exist(this->custom_button_file_c6);

					// wallpaper stuff
					this->wallpaperDayFileExists = check_file_exist("wallpaper.png");
					this->wallpaperNightFileExists = check_file_exist("wallpaper-night.png");
					this->wallpaperClassicDayFileExists = check_file_exist("wallpaper-classic.png");
					this->wallpaperClassicNightFileExists = check_file_exist("wallpaper-classic-night.png");
					this->wallpaperEQFileExists = check_file_exist("wallpaper-eq.png");

					ui_->setupUi(this);
					setupCanService();
					volumeButon_ = new QPushButton("", this);
					volumeButon_->setFixedSize(80, 80);
					QIcon icon(":/sound.png");
					volumeButon_->setIcon(icon);
					volumeButon_->setIconSize(QSize(48, 48)); // Ajuste la taille de l'image
					volumeButon_->setStyleSheet("background-color: rgba(132, 149, 169, 50); color: white; border-radius: 40px;");
					volumeButon_->show();
					volumeButon_->raise();

					volumeSlider_ = new QSlider(Qt::Vertical, this);
					volumeSlider_->setRange(0, 100);
					volumeSlider_->setFixedSize(80, 400);
					volumeSlider_->setStyleSheet(
						"QSlider::groove:vertical { background: rgba(132, 149, 169, 50); width: 80px; border-radius: 5px; }"
						"QSlider::handle:vertical { background: rgb(132, 149, 169); height: 80px; margin: 0 -10px; border-radius: 15px; }"
						"QSlider::add-page:vertical { background: rgb(132, 149, 169); border-radius: 5px; }"
					);
					volumeSlider_->hide();
					volumeSlider_->raise();

					settingsPage_ = new SettingsWindow(configuration, this);
					heatingWindow_ = new HeatingWindow(configuration, this);

					ui_->menuStacked->addWidget(settingsPage_);
					ui_->menuStacked->addWidget(heatingWindow_);
					// Configure window attributes to prevent ghosting
					this->setAttribute(Qt::WA_OpaquePaintEvent, true);
					this->setAttribute(Qt::WA_NoSystemBackground, false);
					this->setAutoFillBackground(true);

					volumeTimer_ = new QTimer(this);
					volumeTimer_->setSingleShot(true);

					connect(volumeTimer_, &QTimer::timeout, [this]() {
						volumeSlider_->hide();
						});

					connect(ui_->pushButtonHome, &QPushButton::clicked, this, &MainWindow::openHome);
					connect(ui_->pushButtonSettings, &QPushButton::clicked, this, &MainWindow::openSettings);
					connect(ui_->pushButtonHeatting, &QPushButton::clicked, this, &MainWindow::openHeating);
					connect(ui_->pushButtonVolume, &QPushButton::clicked, this, &MainWindow::showVolumeSlider);
					connect(ui_->pushButtonBluetooth, &QPushButton::clicked, this, &MainWindow::setPairable);
					connect(volumeSlider_, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
					connect(volumeButon_, &QPushButton::clicked, this, &MainWindow::showVolume);


					ui_->pushButtonBluetooth->hide();
					ui_->labelBluetoothPairable->hide();

					// by default hide media player

					ui_->dcRecording->hide();

					if (!configuration->showNetworkinfo()) {
						ui_->networkInfo->hide();
					}


					QTimer* timer = new QTimer(this);
					connect(timer, SIGNAL(timeout()), this, SLOT(showTime()));
					timer->start(1000);

					ui_->btDevice->hide();

					// check if a device is connected via bluetooth
					if (std::ifstream("/tmp/btdevice")) {
						if (ui_->btDevice->isVisible() == false || ui_->btDevice->text().simplified() == "") {
							QString btdevicename = configuration_->readFileContent("/tmp/btdevice");
							ui_->btDevice->setText(btdevicename);
							ui_->btDevice->show();
						}
					}
					else {
						if (ui_->btDevice->isVisible() == true) {
							ui_->btDevice->hide();
						}
					}

					// as default hide brightness slider
					ui_->BrightnessSliderControl->hide();

					// as default hide volume slider player


					// as default hide muted button

					if (std::ifstream("/tmp/temp_recent_list") || std::ifstream("/tmp/mobile_hotspot_detected")) {
					}
					else {
					}


					// set brightness slider attribs from cs config
					ui_->horizontalSliderBrightness->setMinimum(configuration->getCSValue("BR_MIN").toInt());
					ui_->horizontalSliderBrightness->setMaximum(configuration->getCSValue("BR_MAX").toInt());
					ui_->horizontalSliderBrightness->setSingleStep(configuration->getCSValue("BR_STEP").toInt());
					ui_->horizontalSliderBrightness->setTickInterval(configuration->getCSValue("BR_STEP").toInt());

					// run monitor for custom brightness command if enabled in crankshaft_env.sh
					if (std::ifstream("/tmp/custombrightness")) {
						this->customBrightnessControl = true;
					}

					// read param file
					if (std::ifstream("/boot/crankshaft/volume")) {
						// init volume
						QString vol = QString::number(configuration_->readFileContent("/boot/crankshaft/volume").toInt());
						volumeSlider_->setValue(vol.toInt());
					}


					// use big clock in classic gui?
					if (configuration->showBigClock()) {
						this->UseBigClock = true;
					}
					else {
						this->UseBigClock = false;
					}

					// clock viibility by settings
					if (!configuration->showClock()) {
						ui_->Digital_clock->hide();
						this->NoClock = true;
					}
					else {
						this->NoClock = false;
						if (this->UseBigClock) {
							if (oldGUIStyle) {
								ui_->Digital_clock->hide();
							}
						}
						else {
							ui_->Digital_clock->show();
						}
					}

					// hide brightness button if eanbled in settings
					if (configuration->hideBrightnessControl()) {
						ui_->BrightnessSliderControl->hide();
						// also hide volume button cause not needed if brightness not visible
						ui_->pushButtonVolume->hide();
					}

					// init alpha values
					MainWindow::updateAlpha();

					player = new QMediaPlayer(this);
					playlist = new QMediaPlaylist(this);
					connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::on_positionChanged);
					connect(player, &QMediaPlayer::stateChanged, this, &MainWindow::on_StateChanged);


					this->musicfolder = QString::fromStdString(configuration->getMp3MasterPath());
					this->albumfolder = QString::fromStdString(configuration->getMp3SubFolder());



					player->setPlaylist(this->playlist);
					this->currentPlaylistIndex = configuration->getMp3Track();

					watcher = new QFileSystemWatcher(this);
					watcher->addPath("/media/USBDRIVES");
					connect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::setTrigger);

					watcher_tmp = new QFileSystemWatcher(this);
					watcher_tmp->addPath("/tmp");
					connect(watcher_tmp, &QFileSystemWatcher::directoryChanged, this, &MainWindow::tmpChanged);

					// Experimental test code
					localDevice = new QBluetoothLocalDevice(this);

					connect(localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)),
						this, SLOT(hostModeStateChanged(QBluetoothLocalDevice::HostMode)));

					hostModeStateChanged(localDevice->hostMode());
					updateNetworkInfo();
				}

				MainWindow::~MainWindow()
				{
					delete ui_;
				}

			}
		}
	}
}

void f1x::openauto::autoapp::ui::MainWindow::setupCanService() {
	qRegisterMetaType<f1x::openauto::autoapp::service::CanMessage>("CanMessage");
	m_canThread = new QThread(this);
	m_canService = new f1x::openauto::autoapp::service::CanService();

	if (m_canService->init("can0")) {
		m_canService->moveToThread(m_canThread);
		
		connect(m_canThread, &QThread::started, m_canService, &f1x::openauto::autoapp::service::CanService::process);
		connect(m_canService, &f1x::openauto::autoapp::service::CanService::messageReceived,
			this, &MainWindow::onCanMessageReceived);
		connect(m_canThread, &QThread::finished, m_canService, &QObject::deleteLater);

		m_canThread->start();
	}
}

void f1x::openauto::autoapp::ui::MainWindow::onCanMessageReceived(const f1x::openauto::autoapp::service::CanMessage& msg) {
	QString dataHex = msg.data.toHex(' ').toUpper();
	if (msg.id == 0x1CA && msg.data.size() >= 4)
	{
		uint32_t command = qFromBigEndian<uint32_t>(reinterpret_cast<const uchar*>(msg.data.data()));
		switch (command)
		{
			case 0x03100000:
				downVolume();
				break;
			case 0x03200000:
				upVolume();
				break;
		default:
			break;
		}
	}

	std::stringstream ss;
	ss << std::hex << std::uppercase << msg.id;
	std::string idHex = ss.str();

	OPENAUTO_LOG(debug) << "[UI] Message CAN receive ID: " << idHex << "[data="<< dataHex.toStdString() <<"]";
}

void f1x::openauto::autoapp::ui::MainWindow::sendCanMessage(uint32_t idCan, QByteArray data) {
	if (m_canService) {
		m_canService->sendMessage(idCan, data);
	}
}

QWidget* f1x::openauto::autoapp::ui::MainWindow::getVideoWidget() {
	return ui_->telScreen;
}

void f1x::openauto::autoapp::ui::MainWindow::upVolume() {
	volumeSlider_->show();
	volumeSlider_->raise();
	volumeSlider_->setValue(volumeSlider_->value() + 5);
	onVolumeChanged(volumeSlider_->value());
}

void f1x::openauto::autoapp::ui::MainWindow::downVolume() {
	volumeSlider_->show();
	volumeSlider_->raise();
	volumeSlider_->setValue(volumeSlider_->value() - 5);
	onVolumeChanged(volumeSlider_->value());
}

void f1x::openauto::autoapp::ui::MainWindow::showVolume() {
	volumeSlider_->show();
	volumeSlider_->raise();

	volumeTimer_->start(3000);
}

void f1x::openauto::autoapp::ui::MainWindow::onVolumeChanged(int value) {
	if (volumeTimer_) {
		volumeTimer_->start(3000);
	}
	QString command = QString("amixer set Master %1% &").arg(value);
	system(command.toStdString().c_str());
	qDebug() << "change volume at :" << value << "%";
}


void f1x::openauto::autoapp::ui::MainWindow::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	volumeButon_->move(15, this->height() / 2 + 210);
	volumeSlider_->move(15, this->height() / 2 - 200);
	volumeButon_->raise();
	volumeSlider_->raise();
}


void f1x::openauto::autoapp::ui::MainWindow::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
	if (mode != QBluetoothLocalDevice::HostPoweredOff) {
		this->bluetoothEnabled = true;
		ui_->pushButtonBluetooth->show();
		if (std::ifstream("/tmp/bluetooth_pairable")) {
			ui_->labelBluetoothPairable->show();
			ui_->pushButtonBluetooth->hide();
		}
		else {
			ui_->labelBluetoothPairable->hide();
		}
	}
	else {
		this->bluetoothEnabled = false;
		ui_->pushButtonBluetooth->hide();
		ui_->labelBluetoothPairable->hide();
	}
}

void f1x::openauto::autoapp::ui::MainWindow::updateNetworkInfo()
{
	QNetworkInterface wlan0if = QNetworkInterface::interfaceFromName("wlan0");
	if (wlan0if.flags().testFlag(QNetworkInterface::IsUp)) {
		QList<QNetworkAddressEntry> entrieswlan0 = wlan0if.addressEntries();
		if (!entrieswlan0.isEmpty()) {
			QNetworkAddressEntry wlan0 = entrieswlan0.first();
			//qDebug() << "wlan0: " << wlan0.ip();
			ui_->value_ip->setText(wlan0.ip().toString().simplified());
			ui_->value_mask->setText(wlan0.netmask().toString().simplified());
			if (std::ifstream("/tmp/hotspot_active")) {
				ui_->value_ssid->setText(configuration_->getParamFromFile("/etc/hostapd/hostapd.conf", "ssid"));
			}
			else {
				ui_->value_ssid->setText(configuration_->readFileContent("/tmp/wifi_ssid"));
			}
			ui_->value_gw->setText(configuration_->readFileContent("/tmp/gateway_wlan0"));
		}
	}
	else {
		//qDebug() << "wlan0: down";
		ui_->value_ip->setText("");
		ui_->value_mask->setText("");
		ui_->value_gw->setText("");
		ui_->value_ssid->setText("wlan0: down");
	}
}

void f1x::openauto::autoapp::ui::MainWindow::customButtonPressed1()
{
	system(qPrintable(this->custom_button_command_c1 + " &"));
}

void f1x::openauto::autoapp::ui::MainWindow::customButtonPressed2()
{
	system(qPrintable(this->custom_button_command_c2 + " &"));
}

void f1x::openauto::autoapp::ui::MainWindow::customButtonPressed3()
{
	system(qPrintable(this->custom_button_command_c3 + " &"));
}

void f1x::openauto::autoapp::ui::MainWindow::customButtonPressed4()
{
	system(qPrintable(this->custom_button_command_c4 + " &"));
}

void f1x::openauto::autoapp::ui::MainWindow::customButtonPressed5()
{
	system(qPrintable(this->custom_button_command_c5 + " &"));
}

void f1x::openauto::autoapp::ui::MainWindow::customButtonPressed6()
{
	system(qPrintable(this->custom_button_command_c6 + " &"));
}

void f1x::openauto::autoapp::ui::MainWindow::on_horizontalSliderBrightness_valueChanged(int value)
{
	int n = snprintf(this->brightness_str, 5, "%d", value);
	this->brightnessFile = new QFile(this->brightnessFilename);
	this->brightnessFileAlt = new QFile(this->brightnessFilenameAlt);

	if (!this->customBrightnessControl) {
		if (this->brightnessFile->open(QIODevice::WriteOnly)) {
			this->brightness_str[n] = '\n';
			this->brightness_str[n + 1] = '\0';
			this->brightnessFile->write(this->brightness_str);
			this->brightnessFile->close();
		}
	}
	else {
		if (this->brightnessFileAlt->open(QIODevice::WriteOnly)) {
			this->brightness_str[n] = '\n';
			this->brightness_str[n + 1] = '\0';
			this->brightnessFileAlt->write(this->brightness_str);
			this->brightnessFileAlt->close();
		}
	}
	QString bri = QString::number(value);
	ui_->brightnessValueLabel->setText(bri);
}

void f1x::openauto::autoapp::ui::MainWindow::updateAlpha()
{
	int value = configuration_->getAlphaTrans();
	//int n = snprintf(this->alpha_str, 5, "%d", value);

	if (value != this->alpha_current_str) {
		this->alpha_current_str = value;
		double alpha = value / 100.0;
		QString menu_button_style = "QPushButton{outline-style: dotted; outline-color: #92a8d1;  border: none;} QPushButton:focus {border: 2px solid rgba(125,125,125,0.5);}";

		QString alp = QString::number(alpha);
		ui_->pushButtonVolume->setStyleSheet(menu_button_style);
		ui_->pushButtonSettings->setStyleSheet(menu_button_style);
		ui_->pushButtonHeatting->setStyleSheet(menu_button_style);
		ui_->pushButtonHome->setStyleSheet(menu_button_style);
	}
}

void f1x::openauto::autoapp::ui::MainWindow::toggleGUI()
{
	// Force update before toggling to clear any stale content
	this->update();

	f1x::openauto::autoapp::ui::MainWindow::tmpChanged();

	// Force repaint after UI changes
	this->update();
	this->repaint();
}

void f1x::openauto::autoapp::ui::MainWindow::createDebuglog()
{
	system("/usr/local/bin/crankshaft debuglog &");
}

void f1x::openauto::autoapp::ui::MainWindow::setPairable()
{
	system("/usr/local/bin/crankshaft bluetooth pairable &");
}

void f1x::openauto::autoapp::ui::MainWindow::setMute()
{
	system("/usr/local/bin/autoapp_helper setmute &");
}

void f1x::openauto::autoapp::ui::MainWindow::setUnMute()
{
	system("/usr/local/bin/autoapp_helper setunmute &");
}

void f1x::openauto::autoapp::ui::MainWindow::showTime()
{
	QTime time = QTime::currentTime();
	QDate date = QDate::currentDate();
	QString time_text = time.toString("hh : mm");
	this->date_text = date.toString("dd/MM/yyyy");

	ui_->Digital_clock->setText(time_text);
	ui_->Date->setText(this->date_text);

	// check connected devices
	if (localDevice->isValid()) {
		QString localDeviceName = localDevice->name();
		QString localDeviceAddress = localDevice->address().toString();
		QList<QBluetoothAddress> btdevices;
		btdevices = localDevice->connectedDevices();

		int count = btdevices.count();
		if (count > 0) {
			//QBluetoothAddress btdevice = btdevices[0];
			//QString btmac = btdevice.toString();
			//ui_->btDeviceCount->setText(QString::number(count));
			if (ui_->btDevice->isVisible() == false) {
				ui_->btDevice->show();
			}
			if (std::ifstream("/tmp/btdevice")) {
				ui_->btDevice->setText(configuration_->readFileContent("/tmp/btdevice"));
			}
		}
		else {
			if (ui_->btDevice->isVisible() == true) {
				ui_->btDevice->hide();
				ui_->btDevice->setText("BT-Device");
			}
		}
	}
}

void f1x::openauto::autoapp::ui::MainWindow::on_positionChanged(qint64 position)
{

	//Setting the time
	QString time_elapsed, time_total;

	int total_seconds, total_minutes;

	total_seconds = (player->duration() / 1000) % 60;
	total_minutes = (player->duration() / 1000) / 60;

	if (total_minutes >= 60) {
		int total_hours = (total_minutes / 60);
		total_minutes = total_minutes - (total_hours * 60);
		time_total = QString("%1").arg(total_hours, 2, 10, QChar('0')) + ':' + QString("%1").arg(total_minutes, 2, 10, QChar('0')) + ':' + QString("%1").arg(total_seconds, 2, 10, QChar('0'));

	}
	else {
		time_total = QString("%1").arg(total_minutes, 2, 10, QChar('0')) + ':' + QString("%1").arg(total_seconds, 2, 10, QChar('0'));

	}

	//calculate time elapsed
	int seconds, minutes;

	seconds = (position / 1000) % 60;
	minutes = (position / 1000) / 60;

	//if minutes is over 60 then we should really display hours
	if (minutes >= 60) {
		int hours = (minutes / 60);
		minutes = minutes - (hours * 60);
		time_elapsed = QString("%1").arg(hours, 2, 10, QChar('0')) + ':' + QString("%1").arg(minutes, 2, 10, QChar('0')) + ':' + QString("%1").arg(seconds, 2, 10, QChar('0'));
	}
	else {
		time_elapsed = QString("%1").arg(minutes, 2, 10, QChar('0')) + ':' + QString("%1").arg(seconds, 2, 10, QChar('0'));
	}
}

void f1x::openauto::autoapp::ui::MainWindow::openSettings() {
	ui_->menuStacked->setCurrentWidget(settingsPage_);
}

void f1x::openauto::autoapp::ui::MainWindow::openHeating() {
	ui_->menuStacked->setCurrentWidget(heatingWindow_);
}

void f1x::openauto::autoapp::ui::MainWindow::openHome() {
	ui_->menuStacked->setCurrentIndex(0);
}

void f1x::openauto::autoapp::ui::MainWindow::setTrigger()
{
	this->mediacontentchanged = true;


	QTimer::singleShot(10000, this, SLOT(scanFolders()));
}

void f1x::openauto::autoapp::ui::MainWindow::setRetryUSBConnect()
{

	QTimer::singleShot(10000, this, SLOT(resetRetryUSBMessage()));
}

void f1x::openauto::autoapp::ui::MainWindow::on_StateChanged(QMediaPlayer::State state)
{
	if (state == QMediaPlayer::StoppedState || state == QMediaPlayer::PausedState) {
		std::remove("/tmp/media_playing");
	}
	else {
		std::ofstream("/tmp/media_playing");
	}
}


bool f1x::openauto::autoapp::ui::MainWindow::check_file_exist(const char* fileName)
{
	std::ifstream ifile(fileName, std::ios::in);
	// file not ok - checking if symlink
	if (!ifile.good()) {
		QFileInfo linkFile = QString(fileName);
		if (linkFile.isSymLink()) {
			QFileInfo linkTarget(linkFile.symLinkTarget());
			return linkTarget.exists();
		}
		else {
			return ifile.good();
		}
	}
	else {
		return ifile.good();
	}
}

void f1x::openauto::autoapp::ui::MainWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Return) {
		QApplication::postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier));
		QApplication::postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier));
	}
	if (event->key() == Qt::Key_1) {
		QApplication::postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier));
	}
	if (event->key() == Qt::Key_2) {
		QApplication::postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier));
	}
}

void f1x::openauto::autoapp::ui::MainWindow::tmpChanged()
{
	try {
		if (std::ifstream("/tmp/entityexit")) {
			MainWindow::TriggerAppStop();
			std::remove("/tmp/entityexit");
		}
	}
	catch (...) {
		OPENAUTO_LOG(error) << "[OpenAuto] Error in entityexit";
	}

	// check if system is in display off mode (tap2wake)
	if (std::ifstream("/tmp/blankscreen")) {
		if (ui_->centralWidget->isVisible() == true) {
			CloseAllDialogs();
			ui_->centralWidget->hide();
		}
	}
	else {
		if (ui_->centralWidget->isVisible() == false) {
			ui_->centralWidget->show();
		}
	}

	// check if system is in display off mode (tap2wake/screensaver)
	if (std::ifstream("/tmp/screensaver")) {
		if (ui_->headerWidget->isVisible() == true) {
			ui_->headerWidget->hide();
			CloseAllDialogs();
		}
		if (ui_->BrightnessSliderControl->isVisible() == true) {
			ui_->BrightnessSliderControl->hide();
		}
	}
	else {
		if (ui_->headerWidget->isVisible() == false) {
			ui_->headerWidget->show();
		}
	}

	// check if custom command needs black background
	if (std::ifstream("/tmp/blackscreen")) {
		if (ui_->centralWidget->isVisible() == true) {
			ui_->centralWidget->hide();
			this->setStyleSheet("QMainWindow {background-color: rgb(0,0,0);}");
			this->background_set = false;
		}
	}
	else {
		if (this->background_set == false) {
			this->background_set = true;
		}
	}

	// check if bluetooth pairable
	if (this->bluetoothEnabled) {
		if (std::ifstream("/tmp/bluetooth_pairable")) {
			if (ui_->labelBluetoothPairable->isVisible() == false) {
				ui_->labelBluetoothPairable->show();
			}
			if (ui_->pushButtonBluetooth->isVisible() == true) {
				ui_->pushButtonBluetooth->hide();
			}
		}
		else {
			if (ui_->labelBluetoothPairable->isVisible() == true) {
				ui_->labelBluetoothPairable->hide();
			}
			if (ui_->pushButtonBluetooth->isVisible() == false) {
				ui_->pushButtonBluetooth->show();
			}
		}
	}
	else {
		if (ui_->labelBluetoothPairable->isVisible() == true) {
			ui_->labelBluetoothPairable->hide();
		}
		if (ui_->pushButtonBluetooth->isVisible() == true) {
			ui_->pushButtonBluetooth->hide();
		}
	}

	// check if shutdown is external triggered and init clean app exit
	if (std::ifstream("/tmp/external_exit")) {
		f1x::openauto::autoapp::ui::MainWindow::MainWindow::exit();
	}

	this->hotspotActive = check_file_exist("/tmp/hotspot_active");

	// use big clock in classic gui?
	if (this->configuration_->showBigClock()) {
		this->UseBigClock = true;
	}
	else {
		this->UseBigClock = false;
	}

	if (!this->configuration_->showNetworkinfo()) {
		if (ui_->networkInfo->isVisible() == true) {
			ui_->networkInfo->hide();
		}
	}




	MainWindow::updateAlpha();

	// update notify
	this->csmtupdate = check_file_exist("/tmp/csmt_update_available");
	this->udevupdate = check_file_exist("/tmp/udev_update_available");
	this->openautoupdate = check_file_exist("/tmp/openauto_update_available");
	this->systemupdate = check_file_exist("/tmp/system_update_available");

	updateNetworkInfo();
}
