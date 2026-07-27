# 04 — ORM Enhanced 设计

**优先级**: P1(推荐)
**模块**: 改造现有 soul_orm(非新模块)
**依赖**: soul_core, soul_data, soul_cache(可选)

---

## 1. 设计目标

在 v1.6.x ORM 基础上,增强三项关键能力:
1. **查询结果缓存**: 减少重复查询的数据库压力
2. **类型安全条件构造**: 防止 SQL 注入,提升开发体验
3. **Schema 迁移系统**: 管理数据库版本演进

### 1.1 设计原则

1. **向后兼容**: 不修改现有公共接口,通过新增方法/装饰器扩展
2. **可选启用**: 缓存等增强功能默认关闭,显式 opt-in
3. **零运行时开销**: 类型安全在编译期完成,无运行时反射

---

## 2. 查询结果缓存

### 2.1 CachedRepository 装饰器

详见 `02_soul_cache_design.md` 第 4 节。

### 2.2 缓存失效策略

```cpp
// include/soul/orm/cache_policy.h
namespace sc::orm {

class ICachePolicy {
public:
    virtual ~ICachePolicy() = default;
    virtual bool shouldCache(const std::string& sql, const std::vector<QVariant>& params) = 0;
    virtual std::chrono::milliseconds ttl(const std::string& sql) = 0;
    virtual void onWrite(const std::string& table) = 0;  // 写操作触发表的缓存失效
};

class DefaultCachePolicy : public ICachePolicy {
public:
    bool shouldCache(const std::string& sql, const std::vector<QVariant>& params) override {
        // SELECT 缓存,INSERT/UPDATE/DELETE 不缓存
        return sql.substr(0, 6) == "SELECT";
    }
    std::chrono::milliseconds ttl(const std::string& sql) override {
        return std::chrono::minutes(5);
    }
    void onWrite(const std::string& table) override {
        // 标记该表的所有缓存为脏
        m_dirtyTables.insert(table);
    }
private:
    std::unordered_set<std::string> m_dirtyTables;
};

} // namespace sc::orm
```

### 2.3 自动失效

`CachedRepository` 的写操作自动触发缓存失效:

```cpp
Result<void> CachedRepository<T, KeyType>::update(const T& entity) {
    auto result = m_delegate->update(entity);
    if (result.isOk()) {
        auto key = entity.id();
        m_cache->remove(key);
        m_policy->onWrite(T::tableName());
    }
    return result;
}
```

---

## 3. 类型安全条件构造

### 3.1 问题

现有 `QueryWrapper` 使用字符串列名,存在风险:
```cpp
// 危险:列名拼写错误只能在运行时发现
qw.eq("usrname", "alice");  // 应该是 "username"
```

### 3.2 编译期列名

```cpp
// include/soul/orm/column.h
namespace sc::orm {

template<typename Entity, typename FieldType>
class Column {
public:
    constexpr Column(const char* name, FieldType Entity::* member)
        : m_name(name), m_member(member) {}

    constexpr const char* name() const { return m_name; }
    constexpr FieldType Entity::* member() const { return m_member; }

private:
    const char* m_name;
    FieldType Entity::* m_member;
};

// 宏:简化列定义
#define SOUL_COLUMN(entity, field, name) \
    ::sc::orm::Column<entity, decltype(entity::field)>(name, &entity::field)

} // namespace sc::orm
```

### 3.3 类型安全 QueryWrapper

```cpp
// include/soul/orm/typed_query_wrapper.h
namespace sc::orm {

template<typename T>
class TypedQueryWrapper {
public:
    template<typename F>
    TypedQueryWrapper& eq(const Column<T, F>& col, const F& value) {
        m_wrapper.eq(col.name(), QVariant::fromValue(value));
        return *this;
    }

    template<typename F>
    TypedQueryWrapper& gt(const Column<T, F>& col, const F& value) {
        m_wrapper.gt(col.name(), QVariant::fromValue(value));
        return *this;
    }

    // ... 其他操作符

    Result<std::vector<T>> select(Repository<T>& repo) {
        return repo.selectByQuery(m_wrapper);
    }

private:
    QueryWrapper m_wrapper;
};

} // namespace sc::orm
```

### 3.4 使用示例

```cpp
// 实体定义
struct User {
    int64_t id;
    QString username;
    QString email;
    int age;

    static constexpr auto tableName() { return "users"; }
};

// 列定义(编译期)
constexpr auto COL_ID = SOUL_COLUMN(User, id, "id");
constexpr auto COL_USERNAME = SOUL_COLUMN(User, username, "username");
constexpr auto COL_AGE = SOUL_COLUMN(User, age, "age");

// 类型安全查询
TypedQueryWrapper<User> qw;
auto users = qw
    .eq(COL_USERNAME, "alice")
    .gt(COL_AGE, 18)
    .select(userRepository);

// 编译期错误:类型不匹配
// qw.eq(COL_USERNAME, 123);  // 错误:QString vs int
// qw.eq(COL_NONEXISTENT, "x");  // 错误:无此列
```

---

## 4. Schema 迁移系统

### 4.1 迁移模型

```cpp
// include/soul/orm/migration.h
namespace sc::orm {

class Migration {
public:
    Migration(int64_t version, const std::string& description)
        : m_version(version), m_description(description) {}

    virtual ~Migration() = default;

    virtual void up(SchemaBuilder& builder) = 0;
    virtual void down(SchemaBuilder& builder) = 0;

    int64_t version() const { return m_version; }
    const std::string& description() const { return m_description; }

private:
    int64_t m_version;
    std::string m_description;
};

class SchemaBuilder {
public:
    void createTable(const std::string& name, std::function<void(TableBuilder&)> callback);
    void dropTable(const std::string& name);
    void addColumn(const std::string& table, const ColumnDefinition& column);
    void dropColumn(const std::string& table, const std::string& column);
    void renameColumn(const std::string& table, const std::string& from, const std::string& to);
    void createIndex(const std::string& table, const std::vector<std::string>& columns, const std::string& name);
    void dropIndex(const std::string& name);
    void executeSql(const std::string& sql);
};

} // namespace sc::orm
```

### 4.2 迁移管理器

```cpp
// include/soul/orm/migration_manager.h
namespace sc::orm {

class MigrationManager {
public:
    MigrationManager(std::shared_ptr<IDatabaseDriver> driver);

    void registerMigration(std::shared_ptr<Migration> migration);

    Result<int64_t> currentVersion();
    Result<void> migrate(int64_t targetVersion = -1);  // -1 表示最新
    Result<void> rollback(int64_t steps = 1);

private:
    std::shared_ptr<IDatabaseDriver> m_driver;
    std::vector<std::shared_ptr<Migration>> m_migrations;

    Result<void> ensureMigrationsTable();
    Result<void> applyMigration(Migration& migration);
};

} // namespace sc::orm
```

### 4.3 使用示例

```cpp
// 定义迁移
class CreateUsersTable : public Migration {
public:
    CreateUsersTable() : Migration(1, "Create users table") {}

    void up(SchemaBuilder& builder) override {
        builder.createTable("users", [](TableBuilder& t) {
            t.bigIncrements("id").primary();
            t.string("username", 64).unique().notNull();
            t.string("email", 255).unique();
            t.integer("age").nullable();
            t.timestamps();
        });
    }

    void down(SchemaBuilder& builder) override {
        builder.dropTable("users");
    }
};

class AddUserPhoneColumn : public Migration {
public:
    AddUserPhoneColumn() : Migration(2, "Add phone column to users") {}

    void up(SchemaBuilder& builder) override {
        builder.addColumn("users", ColumnDefinition("phone", ColumnType::String, 20).nullable());
    }

    void down(SchemaBuilder& builder) override {
        builder.dropColumn("users", "phone");
    }
};

// 执行迁移
MigrationManager mgr(driver);
mgr.registerMigration(std::make_shared<CreateUsersTable>());
mgr.registerMigration(std::make_shared<AddUserPhoneColumn>());

auto result = mgr.migrate();  // 应用到最新版本
```

### 4.4 SchemaBuilder 实现

```cpp
class TableBuilder {
public:
    ColumnDefinition& bigIncrements(const std::string& name);
    ColumnDefinition& string(const std::string& name, int length = 255);
    ColumnDefinition& integer(const std::string& name);
    ColumnDefinition& boolean(const std::string& name);
    ColumnDefinition& dateTime(const std::string& name);
    ColumnDefinition& text(const std::string& name);
    void timestamps();  // 自动添加 create_time/update_time
    void primary(const std::vector<std::string>& columns);
    void unique(const std::vector<std::string>& columns, const std::string& name = "");
    void index(const std::vector<std::string>& columns, const std::string& name = "");
    void foreign(const std::string& column, const std::string& refTable, const std::string& refColumn);
};
```

---

## 5. 实体反射增强

### 5.1 自动化字段映射

减少手动 `getPropertyImpl`/`setPropertyImpl` 样板代码:

```cpp
// 宏:声明实体反射
#define SOUL_ENTITY(cls) \
    static constexpr const char* tableName() { return #cls; } \
    template<typename Visitor> void accept(Visitor& v) const; \
    template<typename Visitor> void accept(Visitor& v)

// 宏:实现反射
#define SOUL_FIELD(name, value) \
    v(#name, value)

// 使用
struct User {
    SOUL_ENTITY(User);

    int64_t id;
    QString username;

    template<typename Visitor>
    void accept(Visitor& v) const {
        SOUL_FIELD(id, id);
        SOUL_FIELD(username, username);
    }

    template<typename Visitor>
    void accept(Visitor& v) {
        SOUL_FIELD(id, id);
        SOUL_FIELD(username, username);
    }
};
```

### 5.2 通用 CRUD 生成

基于反射,`BaseRepository` 自动生成 INSERT/UPDATE/SELECT 语句,无需手写 SQL:

```cpp
template<typename T>
class BaseRepository {
public:
    Result<T> insert(const T& entity) {
        auto sql = SqlGenerator::generateInsert<T>();
        auto params = EntitySerializer::toParams(entity);
        return m_driver->executeUpdate(sql, params);
    }

    Result<std::vector<T>> selectAll() {
        auto sql = SqlGenerator::generateSelectAll<T>();
        auto result = m_driver->executeQuery(sql);
        return EntitySerializer::fromResult<T>(result);
    }
};
```

---

## 6. CMake 集成

无需新增库,扩展现有 `soul_orm`:

```cmake
# 在现有 soul_orm 中新增文件
set(SOUL_ORM_HEADERS
    ${SOUL_ORM_HEADERS}
    include/soul/orm/column.h
    include/soul/orm/typed_query_wrapper.h
    include/soul/orm/cached_repository.h
    include/soul/orm/cache_policy.h
    include/soul/orm/migration.h
    include/soul/orm/migration_manager.h
    include/soul/orm/schema_builder.h
    include/soul/orm/entity_macros.h
)

set(SOUL_ORM_SOURCES
    ${SOUL_ORM_SOURCES}
    src/soul/orm/migration_manager.cpp
    src/soul/orm/schema_builder.cpp
)

# 如果启用缓存,链接 soul_cache
if(SOUL_ENABLE_ORM_CACHE)
    target_link_libraries(soul_orm PRIVATE soul_cache)
    target_compile_definitions(soul_orm PUBLIC SOUL_ORM_CACHE_ENABLED)
endif()
```

---

## 7. 测试策略

| 测试类 | 覆盖点 |
|--------|--------|
| `TestCachedRepository` | 缓存命中/失效/并发 |
| `TestTypedQueryWrapper` | 类型安全/编译期检查 |
| `TestMigrationManager` | 版本追踪/迁移/回滚 |
| `TestSchemaBuilder` | DDL 生成/多方言适配 |
| `TestEntityMacros` | 反射/序列化/反序列化 |

---

## 8. 范围决策

### 8.1 v1.7.0 范围(建议)

- ✅ `CachedRepository` 装饰器(依赖 SoulCache)
- ✅ 类型安全 `TypedQueryWrapper` + `Column`
- ✅ 实体反射宏
- ⏸️ Schema 迁移系统 — 视工作量决定是否纳入 v1.7.0 或延后到 v1.8.0

### 8.2 待决策项

1. 迁移系统是否纳入 v1.7.0?
2. 类型安全 QueryWrapper 是否完全替代字符串版本?或两者共存?
3. 反射宏是否影响调试体验?

---

## 9. 风险

| 风险 | 缓解 |
|------|------|
| 缓存失效逻辑遗漏导致脏读 | 写操作自动失效;提供 `invalidateTable()` 手动接口 |
| 类型安全 API 增加编译时间 | 模板代码尽量精简;关键实例化放 .cpp |
| 迁移系统在中途失败导致数据库不一致 | 事务包裹;记录失败状态;支持从失败点恢复 |

---

**文档状态**: Draft
**最后更新**: 2026-07-26
