
const char upload_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Device update</title>

  <style>
    body {
      padding: 30px;
      background: #eee;
    }

    header {
      margin-bottom: 5rem;
    }
    html {
      font-size: 62.5%;
    }

    body,
    textarea {
      font-family: 'Arial, Helvetica, sans-serif';
    }

    h1 {
      font-size: 3rem;
    }

    h2 {
      font-size: 1.6rem;
    }

    h3 {
      font-size: 1.2rem;
    }

    form {
      font-size: 1.4rem;
      color: #222;
    }

    form input[type=submit] {
      width: 24rem;
      height: 100%;
      padding: .6rem 1rem;
      font-size: 1.6rem;
      border: 1px solid green;
      background: #fff;
      align-items: flex-start;
      text-align: center;
      cursor: default;
      color: buttontext;
    }

  </style>
</head>

<body>
  <header>
    <h2>Обновление ПО устройства</h2>
  </header>
  <form method="POST" action="/update" enctype="multipart/form-data">
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
</body>
</html>)~";

const char updatesuccess_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Device update</title>

  <style>
    body {
      padding: 30px;
      background: #eee;
    }

    header {
      margin-bottom: 5rem;
    }
    html {
      font-size: 62.5%;
    }

    body,
    textarea {
      font-family: 'Arial, Helvetica, sans-serif';
    }

    h1 {
      font-size: 3rem;
    }

    h2 {
      font-size: 1.6rem;
    }

  </style>
</head>

<body>
  <header>
    <h2>Обновление завершено успешно</h2>
  </header>
</body>
</html>)~";

const char updatefailure_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Device update</title>

  <style>
    body {
      padding: 30px;
      background: #eee;
    }

    header {
      margin-bottom: 5rem;
    }
    html {
      font-size: 62.5%;
    }

    body,
    textarea {
      font-family: 'Arial, Helvetica, sans-serif';
    }

    h1 {
      font-size: 3rem;
    }

    h2 {
      font-size: 1.6rem;
    }

  </style>
</head>

<body>
  <header>
    <h2>Ошибка обновления</h2>
  </header>
</body>
</html>)~";