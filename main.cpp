// main.cpp
//

#include <iostream>
#include <mutex>
#include <memory>

// Политика создания: использует new/delete
template <typename T>
struct DefaultCreation {
    static T* Create() {
        return new T();
    }

    static void Destroy(T* p) {
        delete p;
    }
};

// Политика потокобезопасности: без блокировок (для однопоточных приложений)
struct SingleThreaded {
    struct Lock {
        Lock(...) {}
    };
};

// Политика потокобезопасности: синхронизация через mutex (для многопоточных приложений)
struct MultiThreaded {
    struct Lock {
        Lock(std::mutex& m) : guard(m) {}
        std::lock_guard<std::mutex> guard;
    };

    static std::mutex& GetMutex() {
        static std::mutex mtx;
        return mtx;
    }
};

// Шаблон Singleton с ортогональными стратегиями
template <
        typename T,
        template <typename> class CreationPolicy = DefaultCreation,
        typename ThreadingModel = SingleThreaded
>
class Singleton {
public:
    // Метод доступа к экземпляру Singleton
    static T& Instance() {
        // Блокировка, если требуется
        typename ThreadingModel::Lock lock(GetMutexIfAny());

        if (!ptr_) {
            ptr_ = CreationPolicy<T>::Create();
            std::atexit(&Destroy);  // Очистка при завершении программы
        }

        return *ptr_;
    }

private:
    // Метод для уничтожения экземпляра при завершении программы
    static void Destroy() {
        CreationPolicy<T>::Destroy(ptr_);
        ptr_ = nullptr;
    }

    // Вспомогательная функция для получения мьютекса или пустышки
    static auto& GetMutexIfAny() {
        if constexpr (std::is_same_v<ThreadingModel, MultiThreaded>) {
            return ThreadingModel::GetMutex();
        } else {
            return *(static_cast<std::mutex*>(nullptr)); // Не используется
        }
    }

    static T* ptr_;  // Указатель на экземпляр
};

// Статическая переменная для хранения указателя
template <typename T, template <typename> class CP, typename TM>
T* Singleton<T, CP, TM>::ptr_ = nullptr;

// === Пример использования ===
class Logger {
public:
    void Log(const std::string& msg) {
        std::cout << "[LOG] " << msg << "\n";
    }
};

// Определение синглтона для Logger с потокобезопасностью:
using LoggerSingleton = Singleton<Logger, DefaultCreation, MultiThreaded>;

int main() {
    // Оба вызова возвращают один и тот же экземпляр, безопасно для многопоточного контекста
    LoggerSingleton::Instance().Log("Hello, Singleton!");
    LoggerSingleton::Instance().Log("Another message");
    return 0;
}
