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


namespace f1x::openauto::autoapp::ui {

    HeatingWindow::HeatingWindow(configuration::IConfiguration::Pointer configuration, QWidget* parent)
        : QWidget(parent), ui_(new Ui::HeatingWindow), configuration_(std::move(configuration)) {
        
        ui_->setupUi(this);

        connect(ui_->fanLess, &QPushButton::clicked, this, &HeatingWindow::lessFan);
        connect(ui_->fanMore, &QPushButton::clicked, this, &HeatingWindow::moreFan);
        connect(ui_->tempLeftMore, &QPushButton::clicked, this, &HeatingWindow::moreTempLeft);
        connect(ui_->tempLeftLess, &QPushButton::clicked, this, &HeatingWindow::lessTempLeft);
        connect(ui_->tempRightMore, &QPushButton::clicked, this, &HeatingWindow::moreTempRight);
        connect(ui_->tempRightLess, &QPushButton::clicked, this, &HeatingWindow::lessTempRight);

    }

    HeatingWindow::~HeatingWindow()
    {
        delete ui_;
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