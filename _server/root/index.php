<?php
session_start();

define('MAX_BODY_LENGTH', 500);

define('GDLOGIN_URL', 'https://boomlings.com/database/accounts/loginGJAccount.php');
$allowed_servers = array(GDLOGIN_URL);
include 'allowed_servers.php';

if (isset($_GET['noWeb'])) unset($_SERVER['HTTP_USER_AGENT']);

//inputs remember
if (isset($_GET['get'])) {
    $_SESSION['get'] = $_GET['get'];
    $_SESSION['delete'] = $_GET['get'];
    $_SESSION['update'] = $_GET['get'];
    $_SESSION['post'] = $_GET['get'];
}
if (isset($_GET['delete'])) $_SESSION['delete'] = $_GET['delete'];
if (isset($_POST['update'])) $_SESSION['update'] = $_POST['update'];
if (isset($_POST['post'])) $_SESSION['post'] = $_POST['post'];
if (isset($_POST['comment_id'])) $_SESSION['comment_id'] = $_POST['comment_id'];

if (isset($_GET['logout'])) {
    $_SESSION = [];
    header('Location: ./');
    exit;
}

if (isset($_SERVER['HTTP_USER_AGENT'])) {
    include 'style.html';
    echo "
    <script>
    document.addEventListener('DOMContentLoaded', function() {
        const preElements = document.querySelectorAll('pre code');
        
        preElements.forEach(preCode => {
            try {
                const jsonData = JSON.parse(preCode.textContent);
                
                if (Array.isArray(jsonData)) {
                    let newHtml = '';
                    
                    jsonData.forEach(item => {
                        if (item.user && item.body) {
                            const date = new Date(item.id * 1000);
                            const formattedDate = date.toLocaleString();
                            
                            newHtml += `<pre><h3 style=\"margin: 0;\">\${item.user} <f style=\"opacity: 0.5;
\">at \${formattedDate}</f></h3>\${item.body}</pre>`;
                        }
                    });
                    
                    if (newHtml) {
                        const newPre = document.createElement('div');
                        newPre.innerHTML = newHtml;
                        preCode.parentElement.replaceWith(...newPre.childNodes);
                    }
                }
            } catch (e) {
                console.error('Ошибка при обработке JSON:', e);
            }
        });
    });
    </script>
    
    <a id=\"backLnk\" style=\"position: fixed;right: 20px;top: 20px;opacity: 0.5;margin: -10px -4px;\" href=\".\"><input type=\"submit\" value=\"Back\"></a>
    <pre><code>";
}

function explPatch($text) {
    return htmlspecialchars($text, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

function modID($id) {
    return preg_replace('/[^a-zA-Z0-9_\-\.]/', '', $id);
}

function load_json($path) {
    return file_exists($path) ? json_decode(file_get_contents($path), true) : [];
}

function save_json($path, $data) {
    file_put_contents($path, json_encode($data, JSON_PRETTY_PRINT));
}

function generateGJP2($password) {
    return sha1($password . "mI29fmAnxgTs");
}

function check_gd_account($userName, $gjp2) {

    $serverUrl = isset($_SESSION["login_link"]) ? $_SESSION["login_link"] : GDLOGIN_URL;
    if (isset($_POST["login_link"])) {
        $serverUrl = $_POST["login_link"];
        $_SESSION["login_link"] = $serverUrl;
    }

    global $allowed_servers;
    if (!in_array($serverUrl, $allowed_servers)) {
        echo "Invalid server URL! ";
        echo "<code><pre>".print_r($allowed_servers, true)."</pre></code>";
        return false;
    }
    
	$udid = "S" . mt_rand(111111111,999999999) . mt_rand(111111111,999999999) . mt_rand(111111111,999999999) . mt_rand(111111111,999999999) . mt_rand(1,9); //getting accountid
	$sid = mt_rand(111111111,999999999) . mt_rand(11111111,99999999);

    $postData = [
        'userName' => $userName,
        'gjp2' => $gjp2,
        'secret' => 'Wmfv3899gc9',
        'udid' => $udid,
        'sID' => $sid
    ];

    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $serverUrl);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query($postData));
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_HEADER, false);

    $response = curl_exec($ch);
    curl_close($ch);

    if (strpos($response, ',') !== false) {
        $parts = explode(',', $response);
        return [
            'accountID' => $parts[0],
            'userID' => $parts[1]
        ];
    }
    return false;
}

function get_gd_user() {

    // game
    if (isset($_POST['userName']) && isset($_POST['gjp2'])) {
        return [
            'userName' => $_POST['userName'],
            'gjp2' => $_POST['gjp2']
        ];
    }
    
    // web
    if (isset($_SESSION['gd_user']) && isset($_SESSION['gd_gjp2'])) {
        return [
            'userName' => $_SESSION['gd_user'],
            'gjp2' => $_SESSION['gd_gjp2']
        ];
    }
    
    return false;
}

function check_auth() {
    $user = get_gd_user();
    if (!$user) return false;
    
    $account = check_gd_account($user['userName'], $user['gjp2']);
    if (!$account) return false;
    
    return [
        'userName' => $user['userName'],
        'accountID' => $account['accountID'],
        'userID' => $account['userID']
    ];
}

if (isset($_GET['login'])) {
    
    // game
    if (isset($_POST['userName']) && isset($_POST['gjp2'])) {
        $account = check_gd_account($_POST['userName'], $_POST['gjp2']);
        if ($account) {
            exit(json_encode([
                'status' => 'success',
                'accountID' => $account['accountID'],
                'userID' => $account['userID']
            ]));
        } else {
            exit(json_encode([
                'status' => 'error',
                'message' => 'Invalid credentials'
            ]));
        }
    }
    
    // web
    $userName = $_POST['username'];
    $password = $_POST['password'];
    $gjp2 = generateGJP2($password);

    $account = check_gd_account($userName, $gjp2);
    
    if ($account) {
        $_SESSION['gd_user'] = $userName;
        $_SESSION['gd_gjp2'] = $gjp2;
        $_SESSION['gd_account_id'] = $account['accountID'];
        $_SESSION['gd_user_id'] = $account['userID'];
        
        exit(
            isset($_SERVER['HTTP_USER_AGENT'])
                ? "Logged in as $userName (Account ID: {$account['accountID']})"
                : json_encode([
                    'status' => 'success',
                    'accountID' => $account['accountID'],
                    'userID' => $account['userID']
                ])
        );
    } else {
        exit("Invalid Geometry Dash credentials");
    }
}

// ПОЛУЧИТЬ КОММЕНТЫ
if (isset($_GET['get'])) {
    $id = modID($_GET['get']);
    $path = './data/comments/'.$id;
    if (!is_dir($path)) exit();
    
    $rtn = array();
    foreach (scandir($path) as $file) {
        if ($file[0] === '.') continue;
        array_push($rtn, json_decode(file_get_contents("$path/$file")));
    }
    echo json_encode($rtn, JSON_PRETTY_PRINT);
    
    exit();
}

// ДОБАВИТЬ КОММЕНТ
if (isset($_GET['post'])) {
    $id = modID($_POST['post']);
    $body = $_POST['body'] ?? '';
    
    $auth = check_auth();
    if (!$auth) exit("Not authorized");

    if (strlen($body) < 1 || strlen($body) > MAX_BODY_LENGTH) exit("Invalid body");

    $path = './data/comments/'.$id;
    @mkdir($path, 0777, true);
    $cid = time();
    $_SESSION['comment_id'] = $cid; //remember
    file_put_contents("$path/$cid", json_encode([
        'id' => $cid,
        'user' => $auth['userName'],
        'body' => $body,
        'accountID' => $auth['accountID'],
        'userID' => $auth['userID']
    ]));
    exit("Comment posted");
}

// ОБНОВИТЬ КОММЕНТ
if (isset($_GET['update'])) {
    $id = modID($_POST['update']);
    $cid = $_POST['comment_id'] ?? '';
    $body = $_POST['body'] ?? '';
    
    $auth = check_auth();
    if (!$auth) exit("Not authorized");
    
    if (strlen($body) < 1 || strlen($body) > MAX_BODY_LENGTH) exit("Invalid body");

    $path = './data/comments/'.$id.'/'.$cid;
    if (!file_exists($path)) exit("Not found");
    $comment = json_decode(file_get_contents($path), true);
    if ($comment['accountID'] !== $auth['accountID']) exit("Not your comment");

    $comment['body'] = $body;
    file_put_contents($path, json_encode($comment));
    exit("Updated");
}

// УДАЛИТЬ КОММЕНТ
if (isset($_GET['delete'])) {
    $id = modID($_POST['delete']);
    $cid = $_POST['comment_id'] ?? '';
    
    $auth = check_auth();
    if (!$auth) exit("Not authorized");
    
    $path = './data/comments/'.$id.'/'.$cid;
    if (!file_exists($path)) exit("Not found");
    $comment = json_decode(file_get_contents($path), true);
    if ($comment['accountID'] !== $auth['accountID']) exit("Not your comment");
    
    unlink($path);
    exit("Deleted");
}

if (isset($_SERVER['HTTP_USER_AGENT'])) {
    function modIDsList() {
        echo "<datalist id=\"modIDs\">";
        $path = './data/comments';
        foreach (scandir($path) as $file) {
            if ($file[0] === '.') continue;
            echo "<option value=\"$file\">";
        }
        echo "</datalist>";
    }
    function loginLinksList() {
        echo "<datalist id=\"login_links\">";
        global $allowed_servers;
        foreach ($allowed_servers as $server) {
            echo "<option value=\"$server\">";
        }
        echo "</datalist>";
    }
    ?>Welcome to Geode Mod Comment Server</pre></code>

<style>
#backLnk { display: none; }
#formWithToken { <?php echo !isset($_SESSION['gd_user']) ? 'filter: opacity(0.5);' : ''; ?> }
body { display: block!important; }
</style>

<form><?php 
        if (isset($_SESSION['gd_user'])) {
            echo 'Logged in as: '.htmlspecialchars($_SESSION['gd_user']).' (Account ID: '.$_SESSION['gd_account_id'].')';
            echo ' <a href="?logout">logout</a>';
        } else {
            echo 'Login with Geometry Dash account!';
        }
        ?>
</form>

<div style="display: flex;gap: 10px;flex-wrap: wrap;">

    <form method="post" action="?login"><h3 style="
    margin: 4px;
">🔑 Geometry Dash Login</h3>
    <input list="login_links" name="login_link" style="
        width: 100%;
        margin: 0;
        padding: 2px 5;
        border-left: transparent;
        border-right: transparent;
        border-top: transparent;
        border-bottom-width: 3px;
        border-radius: 0;
        opacity: 0.5;
    " value="<?php echo isset($_SESSION["login_link"]) 
                    ? $_SESSION["login_link"] : GDLOGIN_URL; 
                ?>">
        <?php loginLinksList(); ?>
        <br>
        <input name="username" placeholder="GD Username" required>
        <input name="password" placeholder="GD Password" type="password" required>
        <input type="submit" value="Login">
    </form>

    <form method="get" style=""><h3>📥 Get Comments</h3>
        <input list="modIDs" name="get" placeholder="Type or select ID..." value="<?php 
            echo $_SESSION['get'] ?? '';
        ?>" required autocomplete="off">
        <?php modIDsList(); ?>
        <input type="submit" value="Get">
    </form>

    <form method="post" action="?delete" id="formWithToken"><h3>❌ Delete</h3>
        <input list="modIDs" name="delete" placeholder="ID" value="<?php 
            echo $_SESSION['delete'] ?? '';
        ?>" required>
        <?php modIDsList(); ?>
        <input name="comment_id" placeholder="Comment ID" value="<?php 
            echo $_SESSION['comment_id'] ?? '';
        ?>" required>
        <input type="submit" value="Delete">
    </form>

    <div style="width: 100%; margin: 0;"></div>

    <form method="post" action="?post" id="formWithToken"><h3>📝 Post</h3>
        <textarea name="body" placeholder="Body" style="width: 100%;" required></textarea>
        <input list="modIDs" name="post" placeholder="ID" value="<?php 
            echo $_SESSION['post'] ?? '';
        ?>" required>
        <?php modIDsList(); ?>
        <input type="submit" value="Post">
    </form>

    <form method="post" action="?update" id="formWithToken"><h3>✏️ Update</h3>
        <textarea name="body" placeholder="Body" style="width: 100%;" required><?php 
            if (isset($_SESSION['update']) and isset($_SESSION['comment_id'])) {
                $id = $_SESSION['update'];
                $cid = $_SESSION['comment_id'];
                $path = './data/comments/'.$id.'/'.$cid;
                if (file_exists($path)) {
                    $comment = json_decode(file_get_contents($path), true);
                    echo $comment['body'];
                }
            }
        ?></textarea>
        <input list="modIDs" name="update" placeholder="ID" value="<?php 
            echo $_SESSION['update'] ?? '';
        ?>" required>
        <?php modIDsList(); ?>
        <input name="comment_id" placeholder="Comment ID" value="<?php 
            echo $_SESSION['comment_id'] ?? '';
        ?>" required>
        <input type="submit" value="Update">
    </form>
</div>
<?php
}
?>