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

#pragma once

#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>
#include <f1x/openauto/autoapp/Service/Can/CanService.hpp>
#include <qthread.h>

#include <QWidget>
#include <QFileDialog>
#include <QComboBox>
#include <QKeyEvent>
#ifdef MAC_OS
#else
#include <sys/sysinfo.h>
#endif

class QCheckBox;
class QTimer;

namespace Ui
{
	class HeatingWindow;
}

namespace f1x::openauto::autoapp::ui
{

	class HeatingWindow : public QWidget
	{
		Q_OBJECT
	public:
		explicit HeatingWindow(configuration::IConfiguration::Pointer configuration, QWidget* parent = nullptr);
		~HeatingWindow() override;

    private slots:
		void moreFan();
		void lessFan();
		void moreTempLeft();
		void lessTempLeft();
		void moreTempRight();
		void lessTempRight();
		void setupCanService();
		void onCanMessageReceived(const f1x::openauto::autoapp::service::CanMessage& msg);
		void sendCanMessage(uint32_t idCan, QByteArray data);

		void changeSeatLeftValue(int value);
		void changeAriscarfLeftValue(int value);
		void changeSeatRightValue(int value);
		void changeAriscarfRightValue(int value);

		void sendValueSeatLeftValue(int value);
		void sendValueAriscarfLeftValue(int value);
		void sendValueSeatRightValue(int value);
		void sendValueAriscarfRightValue(int value);

	private:
		Ui::HeatingWindow* ui_;
		configuration::IConfiguration::Pointer configuration_;
		int fanSpeed = 0;
		int seatTempLeft = 22;
		int seatTempRight = 22;

		f1x::openauto::autoapp::service::CanService* m_canService;
		QThread* m_canThread;
	};
}