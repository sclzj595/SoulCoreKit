# SoulCoreKit 编码规范

## 概述

本文档定义了 SoulCoreKit 的编码风格和命名规范。所有代码必须遵循这些标准。

---

## 命名规范

### 类名

- **规则**：PascalCase
- **示例**：`GlassWidget`, `HttpClient`, `Result`
- **接口**：前缀 `I`
  - **示例**：`INetworkClient`, `IGlassEffect`, `IEventBus`

### 文件名

- **规则**：snake_case
- **示例**：`glass_widget.h`, `http_client.cpp`, `result.h`

### 命名空间

- **根**：`sc`（SoulCore 缩写）
- **子命名空间**：`sc::core`, `sc::ui`, `sc::network` 等
- **工具**：`sc::utils::json`, `sc::utils::file`

### 成员变量

- **规则**：`m_` 前缀 + camelCase
- **示例**：`m_title`, `m_manager`, `m_networkClient`

### 静态变量

- **规则**：`s_` 前缀 + camelCase
- **示例**：`s_instance`, `s_logger`

### 函数名

- **规则**：camelCase
- **示例**：`loadConfig()`, `handleResponse()`, `setTheme()`
- **Getters**：`getXxx()` 或 `xxx()`
- **Setters**：`setXxx()`
- **布尔值**：`isXxx()`, `hasXxx()`, `canXxx()`

### 常量

- **规则**：UPPER_CASE_SNAKE_CASE
- **示例**：`MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`

### 宏

- **规则**：`SC_` 前缀 + UPPER_CASE
- **示例**：`SC_EXPORT`, `SC_INFO`, `SC_WARN`

### 枚举

- **规则**：PascalCase
- **成员**：PascalCase
- **示例**：
  ```cpp
  enum class ColorRole {
      Primary,
      Secondary,
      Success
  };
  ```

---

## 代码格式

### 缩进

- **空格**：4
- **Tab 宽度**：4

### 花括号

- **位置**：K&R 风格（左花括号在同一行）
- **示例**：
  ```cpp
  void function() {
      if (condition) {
          // code
      }
  }
  ```

### 行长度

- **最大**：120 字符

### 空白

- **运算符周围**：`a + b`，不是 `a+b`
- **逗号后**：`func(a, b)`，不是 `func(a,b)`
- **括号内**：`if (x)`，不是 `if(x)`

### 头文件包含

- **顺序**：
  1. 当前源文件对应的头文件
  2. C/C++ 标准头文件
  3. Qt 头文件
  4. SoulCoreKit 头文件
  5. 第三方头文件
- **示例**：
  ```cpp
  #include "button.h"
  
  #include <memory>
  #include <vector>
  
  #include <QWidget>
  #include <QPainter>
  
  #include "soul/core/result.h"
  #include "soul/ui/style.h"
  ```

---

## 头文件保护

- **格式**：`#ifndef SOUL_<MODULE>_<FILE>_H`
- **示例**：
  ```cpp
  #ifndef SOUL_UI_BUTTON_H
  #define SOUL_UI_BUTTON_H
  
  // content
  
  #endif // SOUL_UI_BUTTON_H
  ```

---

## Qt 特定规范

### Q_OBJECT 宏

- **必需**：使用信号/槽的类
- **位置**：`class` 声明后，第一个成员前
- **示例**：
  ```cpp
  class Button : public QWidget {
      Q_OBJECT
  public:
      // ...
  };
  ```

### Signals

- **规则**：camelCase，无返回类型
- **示例**：
  ```cpp
  signals:
      void clicked();
      void textChanged(const QString& text);
  ```

### Slots

- **规则**：camelCase
- **示例**：
  ```cpp
  slots:
      void onButtonClicked();
      void updateText(const QString& text);
  ```

### Properties

- **规则**：使用 Q_PROPERTY 宏进行内省
- **示例**：
  ```cpp
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  ```

---

## C++17 特性

### 必需特性

| 特性 | 用途 |
|------|------|
| `std::unique_ptr` | 所有权管理 |
| `std::shared_ptr` | 共享所有权 |
| `std::optional` | 可选值 |
| `constexpr` | 编译时常量 |
| `inline` | 头文件内函数 |
| `if constexpr` | 编译时条件 |
| `structured bindings` | 元组解包 |
| `std::string_view` | 非所有权字符串引用 |

### 禁止特性

- **用于所有权的裸指针**：使用 `std::unique_ptr` 替代
- **裸数组**：使用 `std::vector` 替代
- **`new`/`delete`**：使用智能指针替代
- **`void*`**：避免无适当抽象的类型擦除

---

## 错误处理

- **返回**：使用 `Result<T>` 而非 `bool`, `int`, `-1`, `nullptr`
- **示例**：
  ```cpp
  // 推荐
  Result<User> getUser(const QString& id);
  
  // 禁止
  User* getUser(const QString& id); // 错误时返回 nullptr
  bool getUser(const QString& id, User& out);
  ```

---

## 文档

### Doxygen 注释

- **必需**：所有公共类、函数和枚举
- **格式**：
  ```cpp
  /**
   * @brief 从文件加载配置
   * 
   * @param path 配置文件路径
   * @return Result<void> 成功或错误
   * @see ConfigSchema
   */
  Result<void> load(const QString& path);
  ```

### 常用标签

| 标签 | 用途 |
|------|------|
| `@brief` | 简要描述 |
| `@param` | 参数描述 |
| `@return` | 返回值描述 |
| `@see` | 相关类/函数 |
| `@note` | 额外说明 |
| `@warning` | 使用警告 |
| `@deprecated` | 已弃用特性 |

---

## 代码组织

### 头文件结构

```cpp
#ifndef SOUL_MODULE_FILE_H
#define SOUL_MODULE_FILE_H

// Includes
#include <memory>
#include <QWidget>

// Forward declarations
namespace sc {
    class Style;
}

// Namespace
namespace sc {
namespace ui {

// Class declaration
class Widget : public QWidget {
    Q_OBJECT
public:
    // Constructor
    Widget(QWidget* parent = nullptr);
    
    // Destructor
    ~Widget() override;
    
    // Public API
    void setStyle(Style* style);
    Style* style() const;
    
signals:
    // Signals
    
slots:
    // Slots
    
protected:
    // Protected methods
    
private:
    // Private members
    Style* m_style;
};

} // namespace ui
} // namespace sc

#endif // SOUL_MODULE_FILE_H
```

### 源文件结构

```cpp
#include "widget.h"

#include "soul/ui/style.h"

namespace sc {
namespace ui {

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , m_style(nullptr)
{
    // Initialization
}

Widget::~Widget() = default;

void Widget::setStyle(Style* style) {
    m_style = style;
}

Style* Widget::style() const {
    return m_style;
}

} // namespace ui
} // namespace sc
```

---

## 最佳实践

### 避免宏

- 使用 `constexpr` 常量替代 `#define`
- 使用 inline 函数替代函数式宏

### 使用 `override`

- 虚函数重写必须使用 `override`
- **示例**：`void paintEvent(QPaintEvent* event) override;`

### 初始化成员

- 使用成员初始化列表
- **示例**：
  ```cpp
  class MyClass {
  public:
      MyClass() : m_value(0), m_name("") {}
  private:
      int m_value;
      QString m_name;
  };
  ```

### 避免全局变量

- 使用受控生命周期的单例
- 优先使用依赖注入

### Const 正确性

- 常量方法标记为 `const`
- 参数使用 `const` 引用
- **示例**：
  ```cpp
  QString name() const;
  void setName(const QString& name);
  ```

---

## 代码审查清单

- ☑ 遵循命名规范
- ☑ 正确的缩进和格式
- ☑ 头文件保护存在
- ☑ 公共 API 有 Doxygen 注释
- ☑ 虚函数重写使用 `override` 关键字
- ☑ 成员变量已初始化
- ☑ 使用 `Result<T>` 进行错误处理
- ☑ 业务代码中无裸 `new`/`delete`
- ☑ 头文件包含顺序正确
- ☑ 信号/槽类有 `Q_OBJECT` 宏