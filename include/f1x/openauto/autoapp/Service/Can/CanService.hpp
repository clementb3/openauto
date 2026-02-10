#pragma once

#include <QObject>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <unistd.h>
#include <f1x/openauto/Common/Log.hpp>


namespace f1x::openauto::autoapp::service {
	struct CanMessage {
		uint32_t id;
		QByteArray data;
	};

	class CanService : public QObject {
		Q_OBJECT
	public:
		explicit CanService(QObject* parent = nullptr);
		virtual ~CanService();

		bool init(const std::string& interfaceName);
		void stop();

	public slots:
		void sendMessage(uint32_t id, const QByteArray& data);
		void process();

	signals:
		void messageReceived(const CanMessage& msg);

	private:
		int _socket;
		std::atomic<bool> _running;
	};
}
