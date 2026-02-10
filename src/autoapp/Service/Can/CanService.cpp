#pragma once

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Service/Can/CanService.hpp>

namespace f1x::openauto::autoapp::service {

    f1x::openauto::autoapp::service::CanService::CanService(QObject* parent)
        : QObject(parent)
        , _socket(-1)
        , _running(false)
    {
    }

    f1x::openauto::autoapp::service::CanService::~CanService()
    {
        stop();
    }
    
    bool CanService::init(const std::string& interfaceName)
    {
        struct sockaddr_can addr;
        struct ifreq ifr;

        if ((_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
            OPENAUTO_LOG(error) << "[CanService] impossible to create can socket.";
            return false;
        }

        std::strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ);
        if (ioctl(_socket, SIOCGIFINDEX, &ifr) < 0) {
            OPENAUTO_LOG(error) << "[CanService] interface " << interfaceName << " not found.";
            close(_socket);
            _socket = -1;
            return false;
        }

        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            OPENAUTO_LOG(error) << "[CanService] bind error " << interfaceName;
            close(_socket);
            _socket = -1;
            return false;
        }

        OPENAUTO_LOG(info) << "[CanService] init on " << interfaceName;
        return true;
    }

    void CanService::stop()
    {
        _running = false;
        if (_socket != -1) {
            close(_socket);
            _socket = -1;
        }
    }

    void CanService::sendMessage(uint32_t id, const QByteArray& data)
    {
        if (_socket < 0) return;

        struct can_frame frame;
        frame.can_id = id;
        frame.can_dlc = static_cast<__u8>(std::min((int)data.size(), 8));
        std::memcpy(frame.data, data.data(), frame.can_dlc);

        if (write(_socket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            OPENAUTO_LOG(error) << "[CanService] errors send ID: " << std::hex << id;
        }
    }

    void CanService::process()
    {
        if (_socket < 0) return;

        _running = true;
        struct can_frame frame;

        OPENAUTO_LOG(info) << "[CanService] wait messages can bus.";

        while (_running) {
            int nbytes = read(_socket, &frame, sizeof(struct can_frame));

            if (nbytes > 0) {
                CanMessage msg;
                msg.id = frame.can_id;
                msg.data = QByteArray(reinterpret_cast<const char*>(frame.data), frame.can_dlc);

                emit messageReceived(msg);
            }
            else if (nbytes < 0 && errno != EINTR) {
                OPENAUTO_LOG(error) << "[CanService] error read: " << strerror(errno);
                _running = false;
            }
        }
    }
}
