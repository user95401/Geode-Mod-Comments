<?php

define('MAX_BODY_LENGTH', 500);

$ARGON_URL = "https://argon.globed.dev/v1";

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

function validate(int $account_id, string $token, ?int $user_id = null, ?string $username = null) {
    
    //registered valid token
    $tokens = load_json("data/tokens.json");
    if (isset($tokens[$account_id]) && $tokens[$account_id]['token'] === $token) {
        //expired
        if (time() - $tokens[$account_id]['timestamp'] > 43200) {//12 hours
            unset($tokens[$account_id]);
            save_json("data/tokens.json", $tokens);
        }
        else return $username ?? true;
    }

    global $ARGON_URL;
    $url = $ARGON_URL . "/validation/" . 
        ($user_id !== null && $username !== null ? "check_strong" : "check") . 
        "?account_id=" . $account_id . 
        "&authtoken=" . urlencode($token);

    if ($user_id !== null && $username !== null) {
        $url .= "&user_id=" . $user_id . "&username=" . urlencode($username);
    }

    $response = file_get_contents($url);

    if ($response === false) {
        throw new Exception("Failed to connect to validation server");
    }

    $http_response_header = $http_response_header ?? [];
    $status_code = explode(' ', $http_response_header[0] ?? '')[1] ?? 0;

    if ($status_code != 200) {
        throw new Exception("Error from argon (code $status_code): $response");
    }

    $resp = json_decode($response, true);

    if ($user_id !== null && $username !== null) {
        // Strong validation logic
        $strong_valid = $resp["valid"];
        $weak_valid = $resp["valid_weak"];
        
        if (!$weak_valid) {
            throw new Exception("Invalid token: " . $resp["cause"]);
        }
        
        if (!$strong_valid) {
            throw new Exception("Mismatched username, please refresh login in account settings");
        }
        
        //save token
        $tokens[$account_id] = [
            'token' => $token,
            'timestamp' => time()
        ];
        save_json("data/tokens.json", $tokens);
        return $resp["username"];
    } else {
        // Basic validation logic
        if (!$resp["valid"]) {
            throw new Exception("Invalid token: " . $resp["cause"]);
        }
        //save token
        $tokens[$account_id] = [
            'token' => $token,
            'timestamp' => time()
        ];
        save_json("data/tokens.json", $tokens);
        return true;
    }
}

//get comments
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

//add comment
if (isset($_GET['post'])) {
    if (!isset($_POST['post'])) exit("The 'post' parameter is required");
    if (!isset($_POST['body'])) exit("The 'body' parameter is required");
    if (!isset($_POST['account_id'])) exit("The 'account_id' parameter is required");
    if (!isset($_POST['user_id'])) exit("The 'user_id' parameter is required");
    if (!isset($_POST['username'])) exit("The 'username' parameter is required");
    if (!isset($_POST['token'])) exit("The 'token' parameter is required");
    
    try {
        validate($_POST['account_id'], $_POST['token'], $_POST['user_id'], $_POST['username']);
    } catch (Exception $e) {
        exit("Error: " . $e->getMessage());
    }
    
    $id = modID($_POST['post']);
    $body = $_POST['body'] ?? '';
    
    if (strlen($body) < 1 || strlen($body) > MAX_BODY_LENGTH) exit("Invalid body");

    $path = './data/comments/'.$id;
    @mkdir($path, 0777, true);
    $cid = time();
    $_SESSION['comment_id'] = $cid; //remember
    file_put_contents("$path/$cid", json_encode([
        'id' => $cid,
        'user' => $_POST['username'],
        'body' => $body,
        'accountID' => $_POST['account_id'],
        'userID' => $_POST['user_id']
    ]));
    exit("Comment posted");
}

//update comment
if (isset($_GET['update'])) {
    if (!isset($_POST['update'])) exit("The 'update' parameter is required");
    if (!isset($_POST['comment_id'])) exit("The 'comment_id' parameter is required");
    if (!isset($_POST['body'])) exit("The 'body' parameter is required");
    if (!isset($_POST['account_id'])) exit("The 'account_id' parameter is required");
    if (!isset($_POST['user_id'])) exit("The 'user_id' parameter is required");
    if (!isset($_POST['username'])) exit("The 'username' parameter is required");
    if (!isset($_POST['token'])) exit("The 'token' parameter is required");
    
    try {
        validate($_POST['account_id'], $_POST['token'], $_POST['user_id'], $_POST['username']);
    } catch (Exception $e) {
        exit("Error: " . $e->getMessage());
    }
    
    $id = modID($_POST['update']);
    $cid = $_POST['comment_id'] ?? '';
    $body = $_POST['body'] ?? '';
    
    if (strlen($body) < 1 || strlen($body) > MAX_BODY_LENGTH) exit("Invalid body");
    
    $path = './data/comments/'.$id.'/'.$cid;
    if (!file_exists($path)) exit("Not found");
    $comment = json_decode(file_get_contents($path), true);
    if ($comment['accountID'] !== $_POST['account_id']) exit("Not your comment");

    $comment['body'] = $body;
    file_put_contents($path, json_encode($comment));
    exit("Updated");
}

//delete comment
if (isset($_GET['delete'])) {
    if (!isset($_POST['delete'])) exit("The 'delete' parameter is required");
    if (!isset($_POST['comment_id'])) exit("The 'comment_id' parameter is required");
    if (!isset($_POST['account_id'])) exit("The 'account_id' parameter is required");
    if (!isset($_POST['user_id'])) exit("The 'user_id' parameter is required");
    if (!isset($_POST['username'])) exit("The 'username' parameter is required");
    if (!isset($_POST['token'])) exit("The 'token' parameter is required");
    
    try {
        validate($_POST['account_id'], $_POST['token'], $_POST['user_id'], $_POST['username']);
    } catch (Exception $e) {
        exit("Error: " . $e->getMessage());
    }
    
    $id = modID($_POST['delete']);
    $cid = $_POST['comment_id'] ?? '';
    
    $path = './data/comments/'.$id.'/'.$cid;
    if (!file_exists($path)) exit("Not found");
    $comment = json_decode(file_get_contents($path), true);
    if ($comment['accountID'] !== $_POST['account_id']) exit("Not your comment");
    
    unlink($path);
    exit("Deleted");
}

?>