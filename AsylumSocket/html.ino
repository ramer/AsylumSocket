
const char upload_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Обновление устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="wrapper_fixed">
  <header>
    <h2>Обновление ПО устройства</h2>
  </header>
  <form method="POST" action="/update" enctype="multipart/form-data">
    <div class="field-group"><a class="upload" href="/">Назад</a></div>
    <h3>Укажите файл для загрузки</h3>
    <div>
      <input type="checkbox" id="spiffs" name="spiffs" value="1">
      <label for="spiffs">Обновление файловой системы</label>
    </div>
    <br>
    <div>
      <input type="file" name="update">
    </div>
    <br>
    <div>
      <input type="submit" value="Обновить и перезагрузить">
    </div>
  </form>
  </div>
</body>
</html>)~";

const char updatesuccess_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Обновление устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="wrapper_fixed">
    <header>
        <h2>Обновление завершено успешно</h2>
    </header>
    <form>
        <div class="field-group"><a class="upload" href="/">Назад</a></div>
    </form>
  </div>
</body>
</html>)~";

const char updatefailure_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Обновление устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="wrapper_fixed">
    <header>
        <h2>Ошибка обновления</h2>
    </header>
    <form>
        <div class="field-group"><a class="upload" href="/">Назад</a></div>
    </form>
  </div>
</body>
</html>)~";