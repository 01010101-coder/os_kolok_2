# Реализация Singleton с ортогональными стратегиями на C++

## Теоретическая часть

**Singleton** — это паттерн, который гарантирует наличие только одного экземпляра класса и предоставляет глобальную точку доступа к этому экземпляру (например, для логгера или конфигурации). Простая реализация часто не учитывает особенности многозадачности и различные способы хранения экземпляра.

**Policy-based design** (проектирование с использованием стратегий) разделяет логику на независимые блоки, каждый из которых можно изменять или расширять без изменения основного кода:

- **CreationPolicy** — политика создания и уничтожения объекта.
- **ThreadingModel** — модель работы с потоками (с блокировками или без).

Каждый из этих блоков может быть заменен или расширен без изменения основной реализации.

---

## Структура реализации

### 1. CreationPolicy

- `DefaultCreation<T>`: создание объекта через `new T()` и удаление через `delete`.

```cpp
template <typename T>
struct DefaultCreation {
    static T* Create() { return new T(); }
    static void Destroy(T* p) { delete p; }
};
```

_При необходимости можно добавить другие стратегии: использование статического буфера, фабрики, пула объектов и так далее._

### 2. ThreadingModel

- `SingleThreaded`: без блокировок, используется в однопоточных приложениях.
- `MultiThreaded`: синхронизация через `std::mutex` для многопоточных приложений.

```cpp
struct SingleThreaded {
    struct Lock { Lock(...) {} };
};

struct MultiThreaded {
    struct Lock { Lock(std::mutex& m) : guard(m) {} std::lock_guard<std::mutex> guard; };
    static std::mutex& GetMutex() { static std::mutex m; return m; }
};
```

### 3. Шаблон Singleton

```cpp
template <
    typename T,
    template <typename> class CreationPolicy = DefaultCreation,
    typename ThreadingModel = SingleThreaded
>
class Singleton {
public:
    static T& Instance() {
        typename ThreadingModel::Lock lock(getMutex());
        if (!instance_) {
            instance_ = CreationPolicy<T>::Create();
            std::atexit(&Destroy);
        }
        return *instance_;
    }

private:
    static void Destroy() { CreationPolicy<T>::Destroy(instance_); instance_ = nullptr; }
    static auto& getMutex() {
        if constexpr (std::is_same_v<ThreadingModel, MultiThreaded>)
            return MultiThreaded::GetMutex();
        else
            return *reinterpret_cast<std::mutex*>(nullptr);
    }
    static T* instance_;
};

template <typename T, template <typename> class CP, typename TM>
T* Singleton<T, CP, TM>::instance_ = nullptr;
```

- При первом обращении к `Instance()` создается объект и регистрируется функция `Destroy()`, которая удаляет объект при завершении программы.
- Метод `getMutex()` возвращает реальный мьютекс для `MultiThreaded`, а для `SingleThreaded` — "пустышку".

---

## Пример использования

```cpp
class Logger {
public:
    void Log(const std::string& msg) { std::cout << msg << std::endl; }
};

using LoggerSingleton = Singleton<Logger, DefaultCreation, MultiThreaded>;

int main() {
    LoggerSingleton::Instance().Log("Начало работы");
    // ...
}
```

В этом примере `LoggerSingleton` — это потокобезопасный одиночка для логирования.

---

## Как использовать в проекте

1. Включите заголовок шаблона в ваш проект.
2. Выберите нужные политики для вашего синглтона:
   ```cpp
   using ConfigSingleton = Singleton<Config, XMLCreation, SingleThreaded>;
   ```
3. Вызовите `ConfigSingleton::Instance()` в тех местах, где вам нужен доступ к синглтону.

---

Таким образом, паттерн Singleton с ортогональными стратегиями позволяет гибко настраивать поведение создания объекта и синхронизации, что может быть полезно в различных приложениях, включая многозадачные.
