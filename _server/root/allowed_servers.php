<?php //v1
$allowed_servers = array_merge([""

    , 'https://gtps.bccst.ru/accounts/loginGJAccount.php'
    , 'https://gmd.plusgdps.dev/database/accounts/loginGJAccount.php'
    , 'https://www.xynelgdps.xyz/database/accounts/loginGJAccount.php'
    , 'https://silverragdps.mathieuar.fr/accounts/loginGJAccount.php'
    , 'http://146.59.93.5/database/accounts/loginGJAccount.php'
    , 'https://gdps.dimisaio.be/database/accounts/loginGJAccount.php'
    , 'https://voidgdps.com/accounts/loginGJAccount.php'
    , 'https://dgdpsdino.ps.fhgdps.com/accounts/loginGJAccount.php'
    , 'https://ddogdgdps.ps.fhgdps.com/accounts/loginGJAccount.php'
    , 'https://prismx.ps.fhgdps.com/accounts/loginGJAccount.php'
    , 'https://sksurl.ps.fhgdps.com/accounts/loginGJAccount.php'
    , 'https://mcgdps.gcs.icu/accounts/loginGJAccount.php'
    , 'https://gcs.icu/gcsdb/accounts/loginGJAccount.php'

], $allowed_servers); //append new servers here

//update itsself from github repo by opening this script in web
if (!isset($_SESSION)) {
    $raw_file_content = file_get_contents(''
        .'https://raw.githubusercontent.com/'
        .'user95401/Geode-Mod-Comments-v2-IN-DEV'   // repo
        .'/refs/heads/main/'                        // branch
        .'_server/allowed_servers.php'              // file
    );
    file_put_contents(__FILE__, $raw_file_content);
    echo "<code><h1>Allowed servers updated</h1>";
    echo "<pre>".htmlspecialchars($raw_file_content)."</pre>";
}
?>