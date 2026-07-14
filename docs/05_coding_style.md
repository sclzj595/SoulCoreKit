# SoulCoreKit Coding Style

## Overview

This document defines the coding style and naming conventions for SoulCoreKit. All code must adhere to these standards.

---

## Naming Conventions

### Class Names

- **Rule**: PascalCase
- **Example**: `GlassWidget`, `HttpClient`, `Result`
- **Interfaces**: Prefix with `I`
  - **Example**: `INetworkClient`, `IGlassEffect`, `IEventBus`

### File Names

- **Rule**: snake_case
- **Example**: `glass_widget.h`, `http_client.cpp`, `result.h`

### Namespaces

- **Root**: `sc` (SoulCore abbreviation)
- **Sub-namespaces**: `sc::core`, `sc::ui`, `sc::network`, etc.
- **Utils**: `sc::utils::json`, `sc::utils::file`

### Member Variables

- **Rule**: `m_` prefix + camelCase
- **Example**: `m_title`, `m_manager`, `m_networkClient`

### Static Variables

- **Rule**: `s_` prefix + camelCase
- **Example**: `s_instance`, `s_logger`

### Function Names

- **Rule**: camelCase
- **Example**: `loadConfig()`, `handleResponse()`, `setTheme()`
- **Getters**: `getXxx()` or `xxx()`
- **Setters**: `setXxx()`
- **Boolean**: `isXxx()`, `hasXxx()`, `canXxx()`

### Constants

- **Rule**: UPPER_CASE_SNAKE_CASE
- **Example**: `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`

### Macros

- **Rule**: `SC_` prefix + UPPER_CASE
- **Example**: `SC_EXPORT`, `SC_INFO`, `SC_WARN`

### Enums

- **Rule**: PascalCase
- **Member**: PascalCase
- **Example**:
  ```cpp
  enum class ColorRole {
      Primary,
      Secondary,
      Success
  };
  ```

---

## Code Formatting

### Indentation

- **Spaces**: 4
- **Tab width**: 4

### Braces

- **Placement**: K&R style (same line for opening brace)
- **Example**:
  ```cpp
  void function() {
      if (condition) {
          // code
      }
  }
  ```

### Line Length

- **Maximum**: 120 characters

### Whitespace

- **Around operators**: `a + b`, not `a+b`
- **After commas**: `func(a, b)`, not `func(a,b)`
- **Inside parentheses**: `if (x)`, not `if(x)`

### Includes

- **Order**:
  1. Header corresponding to this source file
  2. C/C++ standard headers
  3. Qt headers
  4. SoulCoreKit headers
  5. Third-party headers
- **Example**:
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

## Header Guard

- **Format**: `#ifndef SOUL_<MODULE>_<FILE>_H`
- **Example**:
  ```cpp
  #ifndef SOUL_UI_BUTTON_H
  #define SOUL_UI_BUTTON_H
  
  // content
  
  #endif // SOUL_UI_BUTTON_H
  ```

---

## Qt Specific

### Q_OBJECT Macro

- **Required**: For classes using signals/slots
- **Position**: After `class` declaration, before first member
- **Example**:
  ```cpp
  class Button : public QWidget {
      Q_OBJECT
  public:
      // ...
  };
  ```

### Signals

- **Rule**: camelCase, no return type
- **Example**:
  ```cpp
  signals:
      void clicked();
      void textChanged(const QString& text);
  ```

### Slots

- **Rule**: camelCase
- **Example**:
  ```cpp
  slots:
      void onButtonClicked();
      void updateText(const QString& text);
  ```

### Properties

- **Rule**: Q_PROPERTY macro for introspection
- **Example**:
  ```cpp
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  ```

---

## C++17 Features

### Required Features

| Feature | Usage |
|---------|-------|
| `std::unique_ptr` | Ownership management |
| `std::shared_ptr` | Shared ownership |
| `std::optional` | Optional values |
| `constexpr` | Compile-time constants |
| `inline` | Header-only functions |
| `if constexpr` | Compile-time conditionals |
| `structured bindings` | Tuple unpacking |
| `std::string_view` | Non-owning string references |

### Prohibited Features

- **Raw pointers for ownership**: Use `std::unique_ptr` instead
- **Raw arrays**: Use `std::vector` instead
- **`new`/`delete`**: Use smart pointers instead
- **`void*`**: Avoid type erasure without proper abstraction

---

## Error Handling

- **Return**: `Result<T>` instead of `bool`, `int`, `-1`, `nullptr`
- **Example**:
  ```cpp
  // Recommended
  Result<User> getUser(const QString& id);
  
  // Prohibited
  User* getUser(const QString& id); // returns nullptr on error
  bool getUser(const QString& id, User& out);
  ```

---

## Documentation

### Doxygen Comments

- **Required**: For all public classes, functions, and enums
- **Format**:
  ```cpp
  /**
   * @brief Loads configuration from file
   * 
   * @param path Path to config file
   * @return Result<void> Success or error
   * @see ConfigSchema
   */
  Result<void> load(const QString& path);
  ```

### Common Tags

| Tag | Usage |
|-----|-------|
| `@brief` | Brief description |
| `@param` | Parameter description |
| `@return` | Return value description |
| `@see` | Related classes/functions |
| `@note` | Additional notes |
| `@warning` | Warning about usage |
| `@deprecated` | Deprecated feature |

---

## Code Organization

### Header Structure

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

### Source File Structure

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

## Best Practices

### Avoid Macros

- Use `constexpr` constants instead of `#define`
- Use inline functions instead of function-like macros

### Use `override`

- Always use `override` for virtual function overrides
- **Example**: `void paintEvent(QPaintEvent* event) override;`

### Initialize Members

- Use member initializer lists
- **Example**:
  ```cpp
  class MyClass {
  public:
      MyClass() : m_value(0), m_name("") {}
  private:
      int m_value;
      QString m_name;
  };
  ```

### Avoid Global Variables

- Use singletons with controlled lifecycle
- Prefer dependency injection

### Const Correctness

- Mark const methods with `const`
- Use `const` references for parameters
- **Example**:
  ```cpp
  QString name() const;
  void setName(const QString& name);
  ```

---

## Code Review Checklist

- ☑ Follows naming conventions
- ☑ Proper indentation and formatting
- ☑ Header guards present
- ☑ Doxygen comments for public API
- ☑ `override` keyword used where appropriate
- ☑ Member variables initialized
- ☑ Uses `Result<T>` for error handling
- ☑ No raw `new`/`delete` in business code
- ☑ Includes properly ordered
- ☑ `Q_OBJECT` macro present for signal/slot classes