#ifndef SOUL_SERVER_BEANS_ENDPOINT_H
#define SOUL_SERVER_BEANS_ENDPOINT_H

// ============================================================================
// beans_endpoint.h — DI 容器内省端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/beans,暴露 DI 容器中所有已注册的类型
// (Bean)及其生命周期与初始化状态。
//
// 依赖 di::Container::getRegisteredBeans() 提供的内省能力。
//
// 用法:
//   server.get("/actuator/beans", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(BeansEndpoint::toJson());
//   });

#include "soul/utils/json/json_helper.h"
#include "soul/di/container.h"

#include <QByteArray>

namespace sc {
namespace server {

// ============================================================================
// BeansEndpoint — DI 容器内省端点
// ============================================================================
class BeansEndpoint {
public:
    /// @brief 从 DI 容器导出所有已注册 Bean 为 JSON
    /// @return JSON 格式的 Bean 列表(对标 SpringBoot /actuator/beans)
    static QByteArray toJson() {
        // 从全局 DI 容器获取所有已注册类型信息
        auto beans = di::Container::instance().getRegisteredBeans();

        sc::json::Json root = sc::json::Json::object();
        sc::json::Json contexts = sc::json::Json::object();
        sc::json::Json beanList = sc::json::Json::array();

        // 遍历所有 Bean,构建条目
        for (const auto& bean : beans) {
            sc::json::Json entry = sc::json::Json::object();
            entry["type"] = bean.typeName;
            entry["scope"] = bean.lifetime;
            entry["initialized"] = bean.initialized;
            beanList.push_back(entry);
        }

        // 按 SpringBoot 风格组织: contexts.<contextName>.[]
        contexts["soulCoreKit"] = beanList;
        root["contexts"] = contexts;
        return sc::json::serializePretty(root);
    }
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_BEANS_ENDPOINT_H
