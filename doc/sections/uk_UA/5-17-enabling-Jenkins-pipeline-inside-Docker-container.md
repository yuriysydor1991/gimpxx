## Вмикання конвеєра Jenkins всередині Docker контейнера

Для того щоб увімкнути підтримку конвеєра [Jenkins](https://www.jenkins.io/) всередині спеціально виділеного Docker контейнера необхідно побудувати ціль `jenkins-pipeline-docker-run` за допомогою команд:

```
# в середині кореневої директорії проекту

# Конфігурування проекту для підтримки цілей Docker разом з Jenkins
meson setup build -DENABLE_DOCKER=true -DENABLE_JENKINS_DOCKER_PIPELINE=true

# Побудова і запуск контейнера для конвеєрів Jenkins
# або його перезапуск
meson compile -C build jenkins-pipeline-docker-run
```

Після чого необхідно знайти і запам'ятати пароль до [Jenkins](https://www.jenkins.io/) Web-інтерфейсу у логах першого запуску Jenkins системи, який може виглядати наступним чином:

```
# ...

*************************************************************
*************************************************************
*************************************************************

Jenkins initial setup is required. An admin user has been created and a password generated.
Please use the following password to proceed to installation:

aefdc6a4a2214c298c821236c739165c

This may also be found at: /var/jenkins_home/secrets/initialAdminPassword

*************************************************************
*************************************************************
*************************************************************

# ...
```

Необхідно відвідати локальну адресу `http://127.0.0.1:8080` (порт може змінюватись) за допомогою використовуваного браузера щоб завершити початкову інсталяцію і конфігурацію [Jenkins](https://www.jenkins.io/) системи на Web-сторінках його GUI. Після цього створи на ньому завдання типу *Pipeline* з визначенням *Pipeline script from SCM*, вкажи репозиторій та гілку проекту і встанови *Script Path* у значення `misc/Jenkinsfile`.

Усі Meson скрипти побудови контейнера для конвеєрів перевірки [Jenkins](https://www.jenkins.io/) розміщуються у файлі [meson/dockerers/gimpxx-project-docker-Jenkins-pipeline-runner-target/meson.build](/meson/dockerers/gimpxx-project-docker-Jenkins-pipeline-runner-target/meson.build). Порт для Web доступу контейнера Docker до конфігурації конвеєрів перевірки [Jenkins](https://www.jenkins.io/) може бути змінено за допомогою встановлення нового значення для Meson-опції `JENKINS_PIPELINES_PANEL_HTTP_PORT`.

Рішення про побудову образу та про створення нового контейнера чи перезапуск вже створеного приймається на кроці конфігурації, отож після створення контейнера конфігуруй проект перед кожною побудовою цілі; скрипти запуску нижче виконують обидва кроки.

### Скрипти запуску

Директорія [scripts/docker](/scripts/docker) містить скорочення, які виконують обидві наведені вище команди, отож увесь запуск виконується однією командою:

| Скрипт | Запускає Meson-ціль |
| --- | --- |
| [jenkins-run.sh](/scripts/docker/jenkins-run.sh) | `jenkins-pipeline-docker-run` |
| [jenkins-build.sh](/scripts/docker/jenkins-build.sh) | `jenkins-pipeline-docker-build` |

```
# в середині кореневої директорії проекту

scripts/docker/jenkins-run.sh
```

Кожен з них конфігурує директорію `build/docker` з параметрами `ENABLE_DOCKER` і `ENABLE_JENKINS_DOCKER_PIPELINE` у значенні `true` і потім будує потрібну ціль, так само як і [скрипти запуску Docker контейнера](/doc/sections/uk_UA/5-12-enabling-the-docker-container-build-and-run.md). Meson-ціль `jenkins-pipeline-docker-run` залежить від цілі `jenkins-pipeline-docker-build`, а отже скрипт [jenkins-run.sh](/scripts/docker/jenkins-run.sh) будує і образ теж, а скрипт [jenkins-build.sh](/scripts/docker/jenkins-build.sh) потрібен лише для побудови образу без запуску інтерактивного контейнера.

Кожен додатковий параметр скриптів передається кроку `meson setup`:

```
# в середині кореневої директорії проекту

# зміна порту Web-панелі Jenkins у контейнері
scripts/docker/jenkins-run.sh -DJENKINS_PIPELINES_PANEL_HTTP_PORT=9090
```

Прапорець `--rebuild` обох скриптів встановлює Meson-опцію `JENKINS_PIPELINE_FORCE_REBUILD`, котра стирає контейнер і образ на кроці конфігурації та будує образ наново із повторно завантаженим базовим образом Dockerfile і без використання кешу побудови, отож нічого від попереднього образу не залишається. Повторна конфігурація зберігає параметри попередньої, отож кожен запуск скриптів без цього прапорця знову вимикає опцію:

```
# в середині кореневої директорії проекту

scripts/docker/jenkins-run.sh --rebuild
```

### Інструменти контейнера

Образ [misc/Dockerfile.Jenkins.pipeline.meson.in](/misc/Dockerfile.Jenkins.pipeline.meson.in) містить інструменти, які викликають стадії [misc/Jenkinsfile](/misc/Jenkinsfile) і які постачає його базовий дистрибутив Debian: `git`, `meson` разом з `ninja-build`, `g++`, `pkg-config`, пакети розробки GoogleTest / GoogleMock, `clang-format`, `clang-tidy` та `cppcheck`, а також `flatpak` разом з `flatpak-builder` для стадії `RUN_FLATPAK_PACKAGER`, `rpm` для стадії `RUN_RPM_PACKAGER` (стадії `RUN_DEB_PACKAGER` потрібен лише `dpkg-deb`, наявний на кожній системі на базі Debian) та `doxygen` разом з `graphviz` для побудови документації. Дві стадії пакувальників не можуть пройти всередині цього контейнера, отож залишай їхні параметри вимкненими в ньому і запускай їх натомість на хост-системі: `RUN_SNAP_PACKAGER` потребує `snapcraft`, який постачається лише як snap-пакет і тому вимагає запущеного `snapd`, а `RUN_APPIMAGE_PACKAGER` потребує `appimagetool`, якого не надає жоден пакет Debian і який працює через FUSE. Стадії `RUN_FREEBSD_PACKAGER` та `RUN_WIX_PACKAGER` призначені лише для FreeBSD та MS Windows відповідно.

Конвеєр також виконується на власному примірнику [Jenkins](https://www.jenkins.io/) із завданням, визначеним так само: його агенту потрібні ті ж самі інструменти, і скрипти встановлення пакетів [scripts/packages](/scripts/packages) встановлюють усі з них, окрім інструментів пакувальників.
