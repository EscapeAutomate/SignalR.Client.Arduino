#include "signalrclient/hub_protocol.h"
#include "signalrclient/messagepack_hub_protocol.h"
#include "signalrclient/json_hub_protocol.h"
#include "signalrclient/case_insensitive_comparison_utils.h"
#include "signalrclient/signalr_exception.h"
#include "signalrclient/callback_manager.h"
#include "signalrclient/hub_exception.h"
#include "signalrclient/log_writer.h"
#include "signalrclient/trace_level.h"

#include <unordered_map>
#include <functional>

class SignalRHub
{
private:
	std::unordered_map<std::string, std::function<void(const std::vector<signalr::value>&)>, signalr::case_insensitive_hash, signalr::case_insensitive_equals> m_subscriptions;
	signalr::hub_protocol* hub_protocol;
    signalr::callback_manager m_callback_manager;
    signalr::log_writer* m_logger;
    signalr::trace_level m_log_level;

    void Invoke_hub_method(const std::string& method_name, const std::vector<signalr::value>& arguments,
    const std::string& callback_id, std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept;

public:
    SignalRHub(signalr::log_writer* logger, signalr::trace_level logLevel);
	void Setup(bool useMsgPack);
	void HandleMessage(const std::string& message);
    void On(const std::string& event_name, const std::function<void(const std::vector<signalr::value>&)>& handler);
    void Invoke(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(const signalr::value&, std::exception_ptr)> callback) noexcept;
    void Send(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(std::exception_ptr)> callback) noexcept;

    void Start(std::function<void(std::exception_ptr)> callback) noexcept;
    void Stop(std::function<void(std::exception_ptr)> callback) noexcept;

    const std::string& name() const
    {
        return hub_protocol->name();
    }
};