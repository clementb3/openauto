#pragma once

#include <QObject>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <unistd.h>
#include <f1x/openauto/Common/Log.hpp>

struct CanMessage {
    uint32_t id;
    QByteArray data;
};

class CanService : public QObject {
    Q_OBJECT
public:
    explicit CanService(QObject* parent = nullptr) : QObject(parent), _socket(-1) {}

public slots:
    void process() {
        struct can_frame frame;
        while (true) {
            int nbytes = read(_socket, &frame, sizeof(struct can_frame));
            if (nbytes > 0) {
                CanMessage msg;
                msg.id = frame.can_id;
                msg.data = QByteArray(reinterpret_cast<char*>(frame.data), frame.can_dlc);
                OPENAUTO_LOG(debug) << "[canServcie] can message receive" << " ID : " << std::hex << msg.id << " [Data: " << msg.data.toHex(' ').toUpper().data() << "]";
                emit messageReceived(msg);
            }
        }
    }

signals:
    void messageReceived(CanMessage msg);
    void errorOccurred(QString err);

private:
    int _socket;
};
