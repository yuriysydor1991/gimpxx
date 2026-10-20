**Спроба перенесення графічного редактора GIMP на мову C++**

Оригінальний репозиторій взято з [https://gitlab.gnome.org/GNOME/gimp](https://gitlab.gnome.org/GNOME/gimp), з його останнього стабільного випуску `GIMP_3_2_6`.

Деякі частини репозиторію взято з шаблонного C++ проекту за адресою [https://github.com/yuriysydor1991/cpp-app-template.git](https://github.com/yuriysydor1991/cpp-app-template.git) (спеціалізація `appMeson`), так само як їх бере [darknetxx](https://github.com/yuriysydor1991/darknetxx.git).

**Робота триває. Поки що лише запуск GIMP написано на C++**

# Структура репозиторію

| Шлях | Вміст |
|---|---|
| [`meson.build`](/meson.build), [`meson_options.txt`](/meson_options.txt) | Корінь побудови. Спочатку йдуть опції GIMP (малими літерами), за ними опції шаблону (великими літерами) |
| [`meson`](/meson) | Модулі Meson шаблону (`gimpxx-*`): захист бінарних файлів, санітайзери, перевірки коду, профілювальники, пакувальники |
| [`src/main.cpp`](/src/main.cpp), [`src/app`](/src/app), [`src/log`](/src/log) | Прикладний C++ рівень шаблону, який запускає GIMP |
| [`src/gimp-controller`](/src/gimp-controller) | `gimp_controller::GIMPController` та його класи платформ, колишній `main.c` GIMP |
| [`src/gimp`](/src/gimp) | Оригінальне дерево GIMP разом з підмодулем [`gimp-data`](/src/gimp/gimp-data). Його [`README`](/src/gimp/README) - це README від GIMP |
| [`doc`](/doc) | Розділи документації, діаграми та конфігурація Doxygen |
| [`misc`](/misc), [`scripts`](/scripts) | Конфігурації стилю та перевірок коду, шаблони пакунків та допоміжні скрипти |

Посилання `gimp-data` у корені вказує на `src/gimp/gimp-data`, оскільки файли побудови gimp-data звертаються до своїх зображень відносно кореня проекту верхнього рівня.

# Назви

Проект Meson називається `gimpxx`. Інтерфейс користувача називає його **GIMPxx, заснована на GIMP**: екран запуску, заголовки вікон, вікна вітання та «Про програму», запис стільниці та вивід `--version` і `--help`.

Виконувані файли (`gimp-3.2`, `gimp-console-3.2`), директорії встановлення та налаштувань (`share/gimp/3.0`, `~/.config/GIMP/3.2`) та ідентифікатор програми (`org.gimp.GIMP`) залишаються такими ж, як у GIMP, тож gimpxx поділяє з GIMP додатки, дані та налаштування.

# Запуск GIMP

Файл [`src/main.cpp`](/src/main.cpp) викликає `app::ApplicationFactory::execute`. Фабрика створює `gimp_controller::GIMPController` поточної платформи, колишній `main.c` GIMP ([`src/gimp-controller`](/src/gimp-controller)), і виконує його кроки запуску в початковому порядку функції `main()` GIMP:

1. `GIMPController::init_platform` - командний рядок від ОС, оточення платформи та вікно консолі;
1. `GimpSignalsHandler` - трасування стеку та обробники сигналів GIMP, через `GIMPController::init_signals`;
1. `GIMPController::init_process` - підготовка процесу платформи, оточення GLib та GIMP, журналювання GIMP, переклади та копія аргументів, якою володіє `ApplicationContext`;
1. `CommandLineParser` - пошук опцій детального виводу, роботи без інтерфейсу та версії, а потім розбір опцій GIMP, якими володіє контролер;
1. `ApplicationVersionPrinter` або `Application` - вивід версії, або `GIMPController::run`: вже запущений екземпляр GIMP, перевірка бібліотек та запуск GIMP.

Особливості платформ колишнього `main.c` - це віртуальні методи `GIMPController`, які реалізують `LinuxGIMPController`, `MacOSGIMPController` та `WindowsGIMPController`; Meson будує реалізацію поточної платформи. Кожен виконуваний файл GIMP отримує власну компіляцію контролера, консольний - з макросом `GIMP_CONSOLE_COMPILATION`, так само як він отримував `main.c`. Контролер звертається до C коду GIMP лише через [`gimp-c-includes.h`](/src/gimp-controller/gimp-c-includes.h), який модульні тести замінюють на імітації C функцій GIMP. C++ код журналює лише попередження та помилки, у стандартний потік помилок, і не створює файлу журналу. Компонент `SignalsHandler` шаблону присутній, але не встановлюється, оскільки обробники сигналів GIMP поки що зберігають власну поведінку.

# Побудова

Клонуйте репозиторій разом з підмодулем `gimp-data`:

```
git clone --recurse-submodules https://github.com/yuriysydor1991/gimpxx.git
```

На Ubuntu 26.04 скрипт [`scripts/packages/install-ubuntu-26-04.sh`](/scripts/packages/install-ubuntu-26-04.sh) встановлює залежності побудови. Побудова потребує локалі, відмінної від C чи POSIX, наприклад `en_US.UTF-8`. Далі:

```
meson setup build
meson compile -C build
./build/src/gimp/app/gimp-console-3.2 --version
```

Модульні та компонентні C++ тести вмикаються опціями `-DENABLE_UNIT_TESTS=true -DENABLE_COMPONENT_TESTS=true` і запускаються командою `meson test -C build --suite gimpxx-cpp`.

# Зміст документації

**Даний документ у процесі покращення**

1. [Клонування С++ проекту-шаблону](/doc/sections/uk_UA/1-cloning-the-cxx-gimpxx.md)
1. [Створення форку і заміна оригінального репозиторію](/doc/sections/uk_UA/2-forking-and-replacing-the-origin.md)
1. [Вимоги](/doc/sections/uk_UA/3-requirements.md)
    1. [Обов'язкові інструменти для ОС на базі GNU/Лінукс](/doc/sections/uk_UA/3-1-required-tools-for-the-GNU-Linux-based-OS.md)
    1. [Обов'язкові інструменти для ОС на базі MS Windows](/doc/sections/uk_UA/3-2-required-tools-for-the-MS-Windows-based-OS.md)
    1. [Необов'язкові пакети для тестів](/doc/sections/uk_UA/3-3-optional-for-the-tests.md)
    1. [Необов'язкові пакети для створення документації](/doc/sections/uk_UA/3-4-optional-for-the-documentation.md)
    1. [Необов'язкові пакети для форматування коду](/doc/sections/uk_UA/3-5-optional-for-the-code-formatting.md)
    1. [Необов'язкові пакети для статичного аналізатора коду cppcheck](/doc/sections/uk_UA/3-6-optional-for-the-code-analyzer-cppcheck.md)
    1. [Необов'язкові пакети для статичного аналізатора коду clang-tidy](/doc/sections/uk_UA/3-7-optional-for-the-code-analyzer-with-clang-tidy.md)
    1. [Необов'язкові пакет для перевірки використання пам'яті за допомогою Valgrind](/doc/sections/uk_UA/3-8-optional-for-the-memory-checkwith-Valgrind.md)
    1. [Необов'язкові програми для генерації пакету flatpak](/doc/sections/uk_UA/3-9-optional-for-the-flatpak-packager.md)
    1. [Необов'язкові пакети для запуску контейнера Docker](/doc/sections/uk_UA/3-10-optional-for-docker-container-runs.md)
    1. [Необов'язкові пакети для інтеграції libxml2](/doc/sections/uk_UA/3-11-optional-for-the-libxml2-integration.md)
    1. [Необов'язкові пакети для інтеграції OpenCV](/doc/sections/uk_UA/3-12-optional-for-the-OpenCV-integration.md)
    1. [Необов'язкові пакети для пакувальника snap](/doc/sections/uk_UA/3-13-optional-for-the-snap-packager.md)
    1. [Необов'язкові пакети для пакувальника rpm](/doc/sections/uk_UA/3-14-optional-for-the-rpm-packager.md)
    1. [Необов'язкові пакети для пакувальника WIX](/doc/sections/uk_UA/3-15-optional-for-the-WIX-packager.md)
    1. [Необов'язкові пакети для інтеграції libcurl](/doc/sections/uk_UA/3-16-optional-for-the-libcurl-integration.md)
    1. [Необов'язкові пакети для інтеграції nlohmann JSON](/doc/sections/uk_UA/3-17-optional-for-the-nlohmann-json-integration.md)
    1. [Необов'язкові програми для генерації AppImage-збірки](/doc/sections/uk_UA/3-18-optional-for-the-AppImage-packager.md)
1. [Структура проекту](/doc/sections/uk_UA/4-project-structure.md)
    1. [Діаграми проекту](/doc/sections/uk_UA/4-0-project-diagrams.md)
    1. [Реалізуй код одразу!](/doc/sections/uk_UA/4-1-implement-code-straight-away.md)
    1. [Зміна назви проекту і головного виконуваного файлу](/doc/sections/uk_UA/4-2-changing-the-project-and-executable-name.md)
    1. [Впровадження нових параметрів командного рядка](/doc/sections/uk_UA/4-11-introduction-of-the-custom-command-line-parameters.md)
    1. [Реалізація власних нащадків IApplication](/doc/sections/uk_UA/4-7-implementing-your-onw-IApplication-descendants.md)
    1. [Обробка сигналів операційної системи](/doc/sections/uk_UA/4-8-handling-the-OS-signals.md)
    1. [Версіювання і інші параметри проекту](/doc/sections/uk_UA/4-3-version-tracking-and-other-project-parameters.md)
    1. [Мінімально можливі версії](/doc/sections/uk_UA/4-6-minimal-possible-versions.md)
    1. [Тести проекту](/doc/sections/uk_UA/4-4-project-tests.md)
        1. [Фреймворк тестів Google Test](/doc/sections/uk_UA/4-4-1-google-test.md)
    1. [Розширення](/doc/sections/uk_UA/4-5-extensions.md)
1. [Побудова проекту](/doc/sections/uk_UA/5-project-build.md)
    1. [Побудова за допомогою IDE](/doc/sections/uk_UA/5-1-IDE-build.md)
    1. [Побудова проекту-шаблону через командний рядок](/doc/sections/uk_UA/5-2-command-line-build.md)
    1. Вмикання тестів
        1. [Вмикання юніт-тестів](/doc/sections/uk_UA/5-3-1-enabling-unit-testing.md)
        1. [Вмикання компонентних тестів](/doc/sections/uk_UA/5-3-3-enabling-component-testing.md)
    1. [Побудова документації](/doc/sections/uk_UA/5-4-documentation-build.md)
    1. [Вмикання підтримки встановлення документації](/doc/sections/uk_UA/5-5-configuring-the-documentation-install-support.md)
    1. [Додаткові можливості Meson](/doc/sections/uk_UA/5-19-meson-extra-features.md)
    1. Якість коду та санітайзери
        1. [Вмикання підтримки форматування коду](/doc/sections/uk_UA/5-6-enabling-and-performing-code-formatting-target.md)
        1. [Вмикання підтримки цілі статичного аналізатора коду cppcheck](/doc/sections/uk_UA/5-7-enabling-the-static-code-analyzer-target-with-cppcheck.md)
        1. [Вмикання підтримки статичного аналізатора коду clang-tidy](/doc/sections/uk_UA/5-8-enabling-static-code-analyzer-with-clang-tidy.md)
        1. [Вмикання цілі динамічної перевірки пам'яті Valgrind](/doc/sections/uk_UA/5-9-enabling-the-dynamic-memory-check-target-with-valgrind.md)
        1. [Вмикання перевірок під час виконання програми (sanitizers)](/doc/sections/uk_UA/5-13-enabling-sanitizers.md)
    1. Профайлери
        1. [Вмикання підтримки профілювання з gprof](/doc/sections/uk_UA/5-15-enabling-gprof-profiler-analysis.md)
        1. [Вмикання підтримки профілювання за допомогою vagrind/callgrind](/doc/sections/uk_UA/5-16-enabling-valgrinds-callgrind-profiler-analysis.md)
    1. Контейнери та CI
        1. [Вмикання підтримки запуску Docker-контейнера](/doc/sections/uk_UA/5-12-enabling-the-docker-container-build-and-run.md)
        1. [Запуск конвеєра Jenkins](/doc/sections/uk_UA/5-17-enabling-Jenkins-pipeline-inside-Docker-container.md)
        1. [Конвеєр CI GitHub Actions](/doc/sections/uk_UA/5-29-the-GitHub-Actions-CI-pipeline.md)
        1. [Конвеєр CI/CD GitLab](/doc/sections/uk_UA/5-30-the-GitLab-CI-pipeline.md)
    1. Пакувальники
        1. [Вмикання підтримки генерації flatpak пакету](/doc/sections/uk_UA/5-11-enabling-the-flatpak-package-generation-support.md)
        1. [Вмикання підтримки генерації snap-пакету](/doc/sections/uk_UA/5-22-enabling-snap-package-generation.md)
        1. [Вмикання підтримки генерації DEB-пакету](/doc/sections/uk_UA/5-23-enabling-deb-package-generation.md)
        1. [Вмикання підтримки генерації RPM-пакету](/doc/sections/uk_UA/5-24-enabling-rpm-package-generation.md)
        1. [Вмикання підтримки генерації FreeBSD pkg-пакету](/doc/sections/uk_UA/5-25-enabling-freebsd-pkg-package-generation.md)
        1. [Вмикання підтримки генерації AppImage-збірки](/doc/sections/uk_UA/5-26-enabling-the-AppImage-bundle-generation.md)
        1. [Вмикання підтримки генерації WIX MSI-інсталятора](/doc/sections/uk_UA/5-27-enabling-the-WIX-MSI-installer-generation.md)
    1. Бібліотеки
        1. [Вмикання інтеграції libcurl](/doc/sections/uk_UA/5-14-enabling-libcurl.md)
        1. [Вмикання інтеграції бібліотеки nlohmann JSON](/doc/sections/uk_UA/5-18-enabling-the-nlohmann-json-library.md)
        1. [Вмикання інтеграції libxml2](/doc/sections/uk_UA/5-20-enabling-libxml2.md)
        1. [Вмикання інтеграції OpenCV](/doc/sections/uk_UA/5-21-enabling-OpenCV.md)
    1. [Параметри захисту бінарних файлів](/doc/sections/uk_UA/5-31-the-binary-hardening-options.md)
    1. [Опції супровідника проекту](/doc/sections/uk_UA/5-28-project-maintainer-options.md)
1. [Запуск головного виконуваного файлу](/doc/sections/uk_UA/6-run-the-executable.md)
    1. [Запуск через IDE](/doc/sections/uk_UA/6-1-IDE-run.md)
    1. Запуск з терміналу
        1. [Пошук згенерованого виконуваного бінарного файлу](/doc/sections/uk_UA/6-2-1-searching-for-the-generated-executable.md)
        1. [Запуск згенерованого виконуваного файлу](/doc/sections/uk_UA/6-2-2-starting-the-generated-executable.md)
    1. Запуск тестів
        1. [Запуск тестів через meson test](/doc/sections/uk_UA/6-3-1-run-tests-by-the-ctest.md)
        1. [Ручний запуск тестів](/doc/sections/uk_UA/6-3-2-manual-tests-run.md)
1. [Встановлення](/doc/sections/uk_UA/7-installing.md)
