# 05 — 现有模块增强

**优先级**: P2(可选)
**范围**: 对 v1.6.x 现有模块的增强项,按优先级排序

---

## 1. RPC 测试深度扩展

### 1.1 现状

v1.6.0 引入了 SoulRPC 框架,但测试覆盖不足:
- 缺少并发场景测试
- 缺少错误恢复测试
- 缺少性能基准

### 1.2 增强项

| 测试项 | 覆盖点 | 优先级 |
|--------|--------|--------|
| 并发调度测试 | 1000 个并发 RPC 调用,验证 ServiceDispatcher 线程安全 | 高 |
| 超时处理测试 | 客户端超时/服务端处理超时/网络超时 | 高 |
| 重连恢复测试 | 服务端重启后客户端自动重连 | 中 |
| 序列化兼容性 | 不同版本的序列化数据兼容性 | 中 |
| 性能基准 | 单机 QPS / 延迟 P99 | 中 |

### 1.3 验收标准

- RPC 测试覆盖率从当前 ~60% 提升到 ≥ 85%
- 性能基准: 单机 QPS ≥ 10000,P99 延迟 < 10ms

---

## 2. MQ 真实集成

### 2.1 现状

v1.6.x 的 MQ 模块对 RabbitMQ/Kafka 返回 nullptr,未真实集成。

### 2.2 方案

**RabbitMQ 集成**:
- 使用 `amqp-cpp` 库(FetchContent 引入)
- 实现 `RabbitMqConnection` / `RabbitMqProducer` / `RabbitMqConsumer`
- 支持 pub/sub、work queue、routing key 模式

**Kafka 集成**(可选):
- 使用 `librdkafka`
- 实现 `KafkaConnection` / `KafkaProducer` / `KafkaConsumer`
- 支持 consumer group、partition

### 2.3 测试策略

使用 testcontainers 启动真实 RabbitMQ/Kafka 实例:
```yaml
# tests/integration/docker-compose.yml
services:
  rabbitmq:
    image: rabbitmq:3.12-management
    ports: ["5672:5672", "15672:15672"]
  kafka:
    image: confluentinc/cp-kafka:7.5.0
    ports: ["9092:9092"]
```

### 2.4 风险

- 外部依赖增加构建复杂度 → 通过 CMake option 控制
- CI 环境需要 Docker → 仅在 Linux CI 启用集成测试

---

## 3. HTTP/2 支持

### 3.1 现状

当前 HttpClient 基于 QNetworkAccessManager,默认使用 HTTP/1.1。

### 3.2 方案

Qt 6.5 的 QNetworkAccessManager 已支持 HTTP/2,只需启用:
```cpp
request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
```

### 3.3 增强项

- HttpClient 默认启用 HTTP/2
- 新增 `HttpProtocol` 枚举(HTTP_1_1 / HTTP_2 / AUTO)
- 配置化:`Configuration::network().httpProtocol = HttpProtocol::AUTO`

### 3.4 验收标准

- HTTP/2 连接成功建立(通过 Wireshark 或 response header 验证)
- 多路复用: 单连接并发请求性能优于 HTTP/1.1

---

## 4. OAuth2/OIDC 认证

### 4.1 现状

v1.6.x 的 AuthManager 仅支持简单的 token 管理,不支持 OAuth2 授权码流程。

### 4.2 方案

新增 `OAuth2Client` 类,支持:
- 授权码流程(Authorization Code Flow)
- 客户端凭证流程(Client Credentials Flow)
- PKCE 扩展(移动端/SPA)
- Token 刷新
- OIDC 用户信息端点

### 4.3 接口设计

```cpp
class OAuth2Client {
public:
    struct Config {
        QString authorizationEndpoint;
        QString tokenEndpoint;
        QString userInfoEndpoint;
        QString clientId;
        QString clientSecret;
        QString redirectUri;
        QStringList scopes;
    };

    explicit OAuth2Client(const Config& config);

    Result<QString> getAuthorizationUrl(const QString& state, bool usePkce = true);
    Result<TokenResponse> exchangeCodeForToken(const QString& code, const QString& state);
    Result<TokenResponse> refreshToken(const QString& refreshToken);
    Result<UserInfo> getUserInfo(const QString& accessToken);
    Result<TokenResponse> clientCredentialsFlow();
};
```

### 4.4 风险

- 协议复杂,实现工作量大 → 考虑使用 QtNetwork 的 OAuth2 模块
- 安全性要求高 → 必须经过安全审查

---

## 5. UI 模块增强

### 5.1 测试覆盖

| 组件 | 当前覆盖 | 目标 |
|------|----------|------|
| Theme | 低 | ≥ 80% |
| Style | 低 | ≥ 80% |
| BaseWidget | 中 | ≥ 90% |
| 30+ UI 组件 | 不足 | ≥ 70% |

### 5.2 自动化测试

- 点击状态测试: 模拟鼠标点击,验证状态变化
- 禁用行为测试: setEnabled(false) 后的交互
- 样式表测试: QSS 应用后的视觉回归(截图对比)

---

## 6. 配置模块增强

### 6.1 环境隔离

支持 dev/test/prod 三层配置:
```
config/
├── app.yaml          # 基础配置
├── app.dev.yaml      # 开发覆盖
├── app.test.yaml     # 测试覆盖
└── app.prod.yaml     # 生产覆盖
```

通过 `SOUL_ENV` 环境变量切换:
```cpp
Configuration::instance().load("config/app.yaml", env);
```

### 6.2 环境变量支持

```cpp
// 配置文件中使用环境变量
database:
  host: ${DB_HOST:localhost}
  port: ${DB_PORT:5432}
```

---

## 7. 线程池增强

### 7.1 任务优先级

```cpp
class ThreadPool {
public:
    void start(Task task, Priority priority = Priority::Normal);
    // Priority: High / Normal / Low / Background
};
```

### 7.2 公平调度

支持 FIFO 与优先级队列两种调度策略。

---

## 8. 数据库连接池增强

### 8.1 动态扩容

```cpp
struct PoolConfig {
    int minConnections = 2;
    int maxConnections = 10;
    int scaleThreshold = 8;  // 使用率超过 80% 触发扩容
    int scaleStep = 2;
};
```

### 8.2 可配置超时

- acquireTimeout: 获取连接超时
- idleTimeout: 空闲连接回收超时
- validationTimeout: 连接有效性检查超时

---

## 9. 优先级排序

| 增强项 | 价值 | 风险 | 建议优先级 |
|--------|------|------|------------|
| RPC 测试深度 | 高 | 低 | P2-a(优先) |
| HTTP/2 | 中 | 低 | P2-b |
| 配置环境隔离 | 中 | 低 | P2-c |
| UI 测试覆盖 | 中 | 低 | P2-d |
| MQ 真实集成 | 中 | 高 | P2-e(暂缓) |
| OAuth2/OIDC | 中 | 高 | P2-f(暂缓) |
| 线程池优先级 | 低 | 低 | P3 |
| 连接池动态扩容 | 低 | 中 | P3 |

---

## 10. 待决策项

1. v1.7.0 包含哪些 P2 项?(建议:RPC 测试深度 + HTTP/2 + 配置环境隔离)
2. MQ 真实集成是否延后到 v1.8.0?
3. OAuth2 是否延后到 v1.8.0?

---

**文档状态**: Draft
**最后更新**: 2026-07-26
