#ifndef SOUL_ORM_CODE_GENERATOR_H
#define SOUL_ORM_CODE_GENERATOR_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <functional>

namespace sc {
namespace orm {

struct FieldDefinition {
    QString name;
    QString columnName;
    QString typeName;
    bool isPrimaryKey = false;
    bool isAutoIncrement = false;
    bool isNullable = true;
    QString defaultValue;
};

struct EntityDefinition {
    QString className;
    QString tableName;
    QStringList includes;
    std::vector<FieldDefinition> fields;
};

class CodeGenerator {
public:
    static CodeGenerator& instance() {
        static CodeGenerator inst;
        return inst;
    }

    QString generateEntityHeader(const EntityDefinition& def) {
        QStringList lines;
        lines << "#ifndef SOUL_ENTITY_" + def.className.toUpper() + "_H";
        lines << "#define SOUL_ENTITY_" + def.className.toUpper() + "_H";
        lines << "";
        lines << "#include <QString>";
        lines << "#include <QVariant>";
        lines << "#include <QDateTime>";
        for (const auto& inc : def.includes) {
            lines << "#include " + inc;
        }
        lines << "#include \"soul/orm/entity.h\"";
        lines << "";
        lines << "namespace sc {";
        lines << "namespace orm {";
        lines << "";
        lines << "class " + def.className + " : public Entity<" + def.className + "> {";
        lines << "public:";
        lines << "    SC_TABLE(\"" + def.tableName + "\")";
        lines << "";

        for (const auto& field : def.fields) {
            if (!field.isPrimaryKey) {
                lines << "    SC_FIELD(" + field.typeName + ", " + field.name + ")";
            }
        }

        lines << "";
        lines << "    static TableMeta tableMeta() {";
        lines << "        TableMeta meta;";
        lines << "        meta.tableName = QStringLiteral(\"" + def.tableName + "\");";
        lines << "        meta.primaryKey = \"id\";";

        for (const auto& field : def.fields) {
            lines << "        {";
            lines << "            FieldMeta fm;";
            lines << "            fm.name = \"" + field.name + "\";";
            lines << "            fm.columnName = \"" + field.columnName + "\";";
            lines << "            fm.typeName = \"" + field.typeName + "\";";
            lines << "            fm.isPrimaryKey = " + QString(field.isPrimaryKey ? "true" : "false") + ";";
            lines << "            fm.isAutoIncrement = " + QString(field.isAutoIncrement ? "true" : "false") + ";";
            lines << "            fm.isNullable = " + QString(field.isNullable ? "true" : "false") + ";";
            if (!field.defaultValue.isEmpty()) {
                lines << "            fm.defaultValue = \"" + field.defaultValue + "\";";
            }
            lines << "            meta.fields[\"" + field.name + "\"] = fm;";
            lines << "        }";
        }

        lines << "        return meta;";
        lines << "    }";
        lines << "";

        lines << "    QVariant getPropertyImpl(const QString& name) const {";
        lines << "        if (name == \"id\") return id;";
        for (const auto& field : def.fields) {
            if (!field.isPrimaryKey) {
                lines << "        if (name == \"" + field.name + "\") return " + field.name + ";";
            }
        }
        lines << "        return QVariant();";
        lines << "    }";
        lines << "";

        lines << "    void setPropertyImpl(const QString& name, const QVariant& value) {";
        lines << "        if (name == \"id\") id = value.toString();";
        for (const auto& field : def.fields) {
            if (!field.isPrimaryKey) {
                if (field.typeName == "int") {
                    lines << "        if (name == \"" + field.name + "\") " + field.name + " = value.toInt();";
                } else if (field.typeName == "QString") {
                    lines << "        if (name == \"" + field.name + "\") " + field.name + " = value.toString();";
                } else if (field.typeName == "double" || field.typeName == "qreal") {
                    lines << "        if (name == \"" + field.name + "\") " + field.name + " = value.toDouble();";
                } else if (field.typeName == "bool") {
                    lines << "        if (name == \"" + field.name + "\") " + field.name + " = value.toBool();";
                } else {
                    lines << "        if (name == \"" + field.name + "\") " + field.name + " = value.value<" + field.typeName + ">();";
                }
            }
        }
        lines << "    }";
        lines << "";

        lines << "    static " + def.className + " fromRow(const QSqlRecord& record) {";
        lines << "        " + def.className + " entity;";
        lines << "        entity.id = record.value(\"id\").toString();";
        for (const auto& field : def.fields) {
            if (!field.isPrimaryKey) {
                if (field.typeName == "int") {
                    lines << "        entity." + field.name + " = record.value(\"" + field.columnName + "\").toInt();";
                } else if (field.typeName == "QString") {
                    lines << "        entity." + field.name + " = record.value(\"" + field.columnName + "\").toString();";
                } else if (field.typeName == "double" || field.typeName == "qreal") {
                    lines << "        entity." + field.name + " = record.value(\"" + field.columnName + "\").toDouble();";
                } else if (field.typeName == "bool") {
                    lines << "        entity." + field.name + " = record.value(\"" + field.columnName + "\").toBool();";
                } else {
                    lines << "        entity." + field.name + " = record.value(\"" + field.columnName + "\").value<" + field.typeName + ">();";
                }
            }
        }
        lines << "        entity.createTime = record.value(\"create_time\").toDateTime();";
        lines << "        entity.updateTime = record.value(\"update_time\").toDateTime();";
        lines << "        return entity;";
        lines << "    }";
        lines << "";

        lines << "    static QString sqlSelectAll() { return QStringLiteral(\"SELECT * FROM %1 WHERE deleted = 0\").arg(TABLE_NAME()); }";
        lines << "    static QString sqlInsert() { return QStringLiteral(\"INSERT INTO %1(...) VALUES (... )\").arg(TABLE_NAME()); }";
        lines << "    static QString sqlUpdate() { return QStringLiteral(\"UPDATE %1 SET ... WHERE id = ?\").arg(TABLE_NAME()); }";
        lines << "    static QString sqlDelete() { return QStringLiteral(\"DELETE FROM %1 WHERE id = ?\").arg(TABLE_NAME()); }";
        lines << "";

        lines << "};";
        lines << "";
        lines << "} // namespace orm";
        lines << "} // namespace sc";
        lines << "";
        lines << "#endif";

        return lines.join("\n");
    }

    QString generateRepositoryHeader(const EntityDefinition& def) {
        QStringList lines;
        QString repoName = def.className + "Repository";

        lines << "#ifndef SOUL_REPOSITORY_" + def.className.toUpper() + "_H";
        lines << "#define SOUL_REPOSITORY_" + def.className.toUpper() + "_H";
        lines << "";
        lines << "#include <memory>";
        lines << "#include <vector>";
        lines << "#include \"" + def.className.toLower() + ".h\"";
        lines << "#include \"soul/orm/read_write_repository.h\"";
        lines << "";
        lines << "namespace sc {";
        lines << "namespace orm {";
        lines << "";
        lines << "class " + repoName + " : public ReadWriteRepository<" + def.className + "> {";
        lines << "public:";
        lines << "    " + repoName + "(std::shared_ptr<data::IDbDriver> driver, SqlDialectType dialectType = SqlDialectType::SQLite)";
        lines << "        : ReadWriteRepository<" + def.className + ">(driver, dialectType) {}";
        lines << "";
        lines << "    " + repoName + "(std::shared_ptr<data::IDbDriver> readDriver,";
        lines << "              std::shared_ptr<data::IDbDriver> writeDriver,";
        lines << "              SqlDialectType dialectType = SqlDialectType::SQLite)";
        lines << "        : ReadWriteRepository<" + def.className + ">(readDriver, writeDriver, dialectType) {}";
        lines << "";
        lines << "    Result<std::vector<" + def.className + ">> findAllActive() {";
        lines << "        QueryWrapper qw;";
        lines << "        return find(qw);";
        lines << "    }";
        lines << "";
        lines << "    Result<std::vector<" + def.className + ">> findByField(const QString& field, const QVariant& value) {";
        lines << "        QueryWrapper qw;";
        lines << "        qw.eq(field, value);";
        lines << "        return find(qw);";
        lines << "    }";
        lines << "";
        lines << "    Result<" + def.className + "> findFirstByField(const QString& field, const QVariant& value) {";
        lines << "        QueryWrapper qw;";
        lines << "        qw.eq(field, value);";
        lines << "        return findOne(qw);";
        lines << "    }";
        lines << "";
        lines << "};";
        lines << "";
        lines << "} // namespace orm";
        lines << "} // namespace sc";
        lines << "";
        lines << "#endif";

        return lines.join("\n");
    }

    QString generateServiceHeader(const EntityDefinition& def) {
        QStringList lines;
        QString serviceName = def.className + "Service";
        QString repoName = def.className + "Repository";

        lines << "#ifndef SOUL_SERVICE_" + def.className.toUpper() + "_H";
        lines << "#define SOUL_SERVICE_" + def.className.toUpper() + "_H";
        lines << "";
        lines << "#include <memory>";
        lines << "#include <vector>";
        lines << "#include \"" + def.className.toLower() + "_repository.h\"";
        lines << "#include \"soul/core/result.h\"";
        lines << "";
        lines << "namespace sc {";
        lines << "namespace orm {";
        lines << "";
        lines << "class " + serviceName + " {";
        lines << "public:";
        lines << "    explicit " + serviceName + "(std::shared_ptr<" + repoName + "> repository)";
        lines << "        : m_repository(std::move(repository)) {}";
        lines << "";
        lines << "    Result<std::vector<" + def.className + ">> findAll() {";
        lines << "        return m_repository->findAll();";
        lines << "    }";
        lines << "";
        lines << "    Result<" + def.className + "> findById(const QString& id) {";
        lines << "        return m_repository->findById(id);";
        lines << "    }";
        lines << "";
        lines << "    Result<std::vector<" + def.className + ">> findByField(const QString& field, const QVariant& value) {";
        lines << "        return m_repository->findByField(field, value);";
        lines << "    }";
        lines << "";
        lines << "    Result<" + def.className + "> create(const " + def.className + "& entity) {";
        lines << "        return m_repository->save(entity);";
        lines << "    }";
        lines << "";
        lines << "    Result<" + def.className + "> update(const " + def.className + "& entity) {";
        lines << "        return m_repository->save(entity);";
        lines << "    }";
        lines << "";
        lines << "    Result<void> removeById(const QString& id) {";
        lines << "        return m_repository->removeById(id);";
        lines << "    }";
        lines << "";
        lines << "    Result<int> count() {";
        lines << "        return m_repository->count();";
        lines << "    }";
        lines << "";
        lines << "    Result<bool> exists(const QString& id) {";
        lines << "        return m_repository->existsById(id);";
        lines << "    }";
        lines << "";
        lines << "private:";
        lines << "    std::shared_ptr<" + repoName + "> m_repository;";
        lines << "};";
        lines << "";
        lines << "} // namespace orm";
        lines << "} // namespace sc";
        lines << "";
        lines << "#endif";

        return lines.join("\n");
    }

    QString generateCMakeListsEntry(const EntityDefinition& def) {
        QStringList lines;
        QString lowerName = def.className.toLower();
        lines << "    SOUL_ORM_HEADERS +=";
        lines << "        ${CMAKE_CURRENT_SOURCE_DIR}/include/soul/orm/" + lowerName + ".h";
        lines << "        ${CMAKE_CURRENT_SOURCE_DIR}/include/soul/orm/" + lowerName + "_repository.h";
        lines << "        ${CMAKE_CURRENT_SOURCE_DIR}/include/soul/orm/" + lowerName + "_service.h";
        return lines.join("\n");
    }

    void generateFiles(const EntityDefinition& def,
                       const std::function<void(const QString&, const QString&)>& writeCallback) {
        QString entityHeader = generateEntityHeader(def);
        writeCallback(def.className.toLower() + ".h", entityHeader);

        QString repoHeader = generateRepositoryHeader(def);
        writeCallback(def.className.toLower() + "_repository.h", repoHeader);

        QString serviceHeader = generateServiceHeader(def);
        writeCallback(def.className.toLower() + "_service.h", serviceHeader);
    }

    QString generateCreateTableSql(const EntityDefinition& def) {
        QStringList lines;
        lines << "CREATE TABLE IF NOT EXISTS " + def.tableName + " (";
        lines << "    id TEXT PRIMARY KEY,";
        for (const auto& field : def.fields) {
            if (!field.isPrimaryKey) {
                lines << "    " + field.columnName + " " + mapTypeToSql(field.typeName)
                      + (field.isNullable ? "" : " NOT NULL") + ",";
            }
        }
        lines << "    create_time TEXT NOT NULL,";
        lines << "    update_time TEXT NOT NULL,";
        lines << "    deleted INTEGER NOT NULL DEFAULT 0";
        lines << ")";
        return lines.join("\n");
    }

private:
    CodeGenerator() = default;

    QString mapTypeToSql(const QString& type) {
        if (type == "int" || type == "bool") return "INTEGER";
        if (type == "double" || type == "qreal") return "REAL";
        if (type == "QString") return "TEXT";
        if (type == "QByteArray") return "BLOB";
        return "TEXT";
    }
};

// 编译期代码生成宏
#define ORM_DEFINE_ENTITY(ClassName, TableName) \
    namespace sc { \
    namespace orm { \
    class ClassName; \
    template<> \
    struct EntityTraits<ClassName> { \
        static constexpr const char* tableName = TableName; \
        static constexpr const char* primaryKey = "id"; \
    }; \
    } \
    }

#define ORM_DEFINE_FIELD(ClassName, FieldName, ColumnName, Type) \
    ORM_FIELD_REGISTERER<ClassName, FieldName, ColumnName, Type>

namespace sc {
namespace orm {

template<typename T>
struct EntityTraits;

template<typename T, typename FieldType>
struct FieldTraits;

template<typename T, int Index>
struct FieldAt;

} // namespace orm
} // namespace sc

#endif
