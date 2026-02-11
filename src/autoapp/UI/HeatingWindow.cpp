/*subfolder
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
#include <fstream>
#include <string>
#include <f1x/openauto/autoapp/UI/HeatingWindow.hpp>
#include <qobject.h>
#include <bits/stdint-uintn.h>
#include <qglobal.h>
#include <qendian.h>
#include <sstream>
#include <QBluetoothLocalDevice>
#include <QBluetoothHostInfo>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QProcess>
#include <QStorageInfo>
#include <QTextStream>
#include <QTimer>
#include "ui_heatingwindow.h"
#include <cmath>

namespace f1x::openauto::autoapp::ui {

    HeatingWindow::HeatingWindow(configuration::IConfiguration::Pointer configuration, QWidget* parent)
        : QWidget(parent), ui_(new Ui::HeatingWindow), configuration_(std::move(configuration)) {
        
        ui_->setupUi(this);
        setupCanService();
        connect(ui_->fanLess, &QPushButton::clicked, this, &HeatingWindow::lessFan);
        connect(ui_->fanMore, &QPushButton::clicked, this, &HeatingWindow::moreFan);
        connect(ui_->tempLeftMore, &QPushButton::clicked, this, &HeatingWindow::moreTempLeft);
        connect(ui_->tempLeftLess, &QPushButton::clicked, this, &HeatingWindow::lessTempLeft);
        connect(ui_->tempRightMore, &QPushButton::clicked, this, &HeatingWindow::moreTempRight);
        connect(ui_->tempRightLess, &QPushButton::clicked, this, &HeatingWindow::lessTempRight);
        connect(ui_->heatSeatLeftControl, &QSlider::valueChanged, this, &HeatingWindow::sendValueSeatLeftValue);
        connect(ui_->airScraftLeftControl, &QSlider::valueChanged, this, &HeatingWindow::sendValueAriscarfLeftValue);
        connect(ui_->heatSeatRightControl, &QSlider::valueChanged, this, &HeatingWindow::sendValueSeatRightValue);
        connect(ui_->airScraftRightControl, &QSlider::valueChanged, this, &HeatingWindow::sendValueAriscarfRightValue);

    }

    HeatingWindow::~HeatingWindow()
    {
        delete ui_;
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::setupCanService() {
        qRegisterMetaType<f1x::openauto::autoapp::service::CanMessage>("CanMessage");
        m_canThread = new QThread(this);
        m_canService = new f1x::openauto::autoapp::service::CanService();

        if (m_canService->init("can0")) {
            m_canService->moveToThread(m_canThread);

            connect(m_canThread, &QThread::started, m_canService, &f1x::openauto::autoapp::service::CanService::process);
            connect(m_canService, &f1x::openauto::autoapp::service::CanService::messageReceived,
                this, &HeatingWindow::onCanMessageReceived);
            connect(m_canThread, &QThread::finished, m_canService, &QObject::deleteLater);

            m_canThread->start();
        }
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::onCanMessageReceived(const f1x::openauto::autoapp::service::CanMessage& msg) {
        QString dataHex = msg.data.toHex(' ').toUpper();
        if (msg.id == 210 && msg.data.size() >= 4)
        {
            bool ok;
            QString firstByteHex = dataHex.left(2);
            int firstByteHexValue = firstByteHex.toInt(&ok, 16);

            QString secondByteHex = dataHex.section(' ', 1, 1);
            int secondByteHexValue = firstByteHex.toInt(&ok, 16);

			OPENAUTO_LOG(debug) << "[UI heat] First byte hex value : " << firstByteHex.toStdString();
			OPENAUTO_LOG(debug) << "[UI heat] Econd byte hex value : " << secondByteHex.toStdString();
            switch (firstByteHexValue%20)
            {
            case 18:
                changeSeatLeftValue(3);
                break;
            case 10:
                changeSeatLeftValue(2);
                break;
            case 8:
                changeSeatLeftValue(1);
                break;
			case 0:
                changeSeatLeftValue(0);
                break;
            }
            int arrondi = std::floor(firstByteHexValue / 20);
            switch (arrondi)
            {
            case 3:
                changeAriscarfLeftValue(3);
                break;
            case 2:
                changeAriscarfLeftValue(2);
                break;
            case 1:
                changeAriscarfLeftValue(1);
                break;
			case 0:
                changeAriscarfLeftValue(0);
                break;
            }

            switch (secondByteHexValue %20)
            {
            case 18:
                changeSeatRightValue(3);
                break;
            case 10:
                changeSeatRightValue(2);
                break;
            case 8:
                changeSeatRightValue(1);
                break;
			case 0:
                changeSeatRightValue(0);
                break;
            }
            arrondi = std::floor(secondByteHexValue / 20);
            switch (arrondi)
            {
            case 3:
                changeAriscarfRightValue(3);
                break;
            case 2:
                changeAriscarfRightValue(2);
                break;
            case 1:
                changeAriscarfRightValue(1);
                break;
			case 0:
                changeAriscarfRightValue(0);
                break;
            }
        }

        std::stringstream ss;
        ss << std::hex << std::uppercase << msg.id;
        std::string idHex = ss.str();

        OPENAUTO_LOG(debug) << "[UI heat] Message CAN receive ID: " << idHex << "[data=" << dataHex.toStdString() << "]";
    }


    void f1x::openauto::autoapp::ui::HeatingWindow::sendCanMessage(uint32_t idCan, QByteArray data) {
        if (m_canService) {
            m_canService->sendMessage(idCan, data);
        }
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::changeSeatLeftValue(int value){
		ui_->heatSeatLeftControl->setValue(value);
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::changeAriscarfLeftValue(int value){
		ui_->airScraftLeftControl->setValue(value);
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::changeSeatRightValue(int value){
        ui_->heatSeatRightControl->setValue(value);
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::changeAriscarfRightValue(int value){
        ui_->airScraftRightControl->setValue(value);
    }


    void f1x::openauto::autoapp::ui::HeatingWindow::sendValueSeatLeftValue(int value){
        sendCanMessage(0x02C, QByteArray::fromHex("00 00 08 00"));
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::sendValueAriscarfLeftValue(int value){
        sendCanMessage(0x02C, QByteArray::fromHex("00 00 02 00"));
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::sendValueSeatRightValue(int value){
        sendCanMessage(0x02C, QByteArray::fromHex("00 00 80 00"));
    }

    void f1x::openauto::autoapp::ui::HeatingWindow::sendValueAriscarfRightValue(int value){
        sendCanMessage(0x02C, QByteArray::fromHex("00 00 20 00"));
    }


    void HeatingWindow::moreFan() {
        QString command = QString("crankshaft-heating fan %1 &").arg(fanSpeed);
        system(command.toStdString().c_str());

        if (fanSpeed < 5 )
        {
            fanSpeed++;
        }
		ui_->fanPower->setText(QString::number(fanSpeed));
	}

    void HeatingWindow::lessFan() {
        QString command = QString("crankshaft-heating fan %1 &").arg(fanSpeed);
        system(command.toStdString().c_str());

        if (fanSpeed > 0)
        {
            fanSpeed--;
        }
        ui_->fanPower->setText(QString::number(fanSpeed));

	}

    void HeatingWindow::moreTempLeft() {
        QString command = QString("crankshaft-heating leftSeat %1 &").arg(seatTempLeft);
        system(command.toStdString().c_str());

        if (seatTempLeft < 28)
        {
            seatTempLeft++;
        }
        ui_->tempLeftValue->setText(QString::number(seatTempLeft)+QStringLiteral("°C"));
    }
    void HeatingWindow::lessTempLeft() {
        QString command = QString("crankshaft-heating leftSeat %1 &").arg(seatTempLeft);
        system(command.toStdString().c_str());

        if (seatTempLeft > 16)
        {
            seatTempLeft--;
        }
        ui_->tempLeftValue->setText(QString::number(seatTempLeft) + QStringLiteral("°C"));
    }
    void HeatingWindow::moreTempRight() {
        QString command = QString("crankshaft-heating rightSeat %1 &").arg(seatTempRight);
        system(command.toStdString().c_str());

        if (seatTempRight < 28)
        {
            seatTempRight++;
        }
        ui_->tempRightValue->setText(QString::number(seatTempRight) + QStringLiteral("°C"));
    }
    void HeatingWindow::lessTempRight() {
        QString command = QString("crankshaft-heating rightSeat %1 &").arg(seatTempRight);
        system(command.toStdString().c_str());


        if (seatTempRight > 16)
        {
            seatTempRight--;
        }
        ui_->tempRightValue->setText(QString::number(seatTempRight) + QStringLiteral("°C"));
    }
}