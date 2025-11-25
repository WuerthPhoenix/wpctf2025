<?php

$pdo = new PDO("mysql:host=127.0.0.1;dbname=example", "root", "example");

function mysql_identifier($s){
    return "`".str_replace("`", "``", $s)."`";
}

switch($_GET['action']){
    case 'insert':
        die("Sorry not implemented");
        break;
    case 'search':
        if (!isset($_GET['where']) || !is_string($_GET['where']) || !isset($_GET['col']) || !is_string($_GET['col'])) {
            die("Invalid input");
        }

        if(strlen($_GET['where']) > 60 || strlen($_GET['col']) > 40) {
            die("Too long");
        }

        $col = mysql_identifier($_GET['col']);
        $where = explode(",", $_GET['where']);
        
        if(count($where) > 2) {
            die("Too many clauses!");
        }

        $params = [];

        foreach ($where as &$clause) {
            $raw = explode(':',$clause);

            $clause = mysql_identifier($raw[0]) . " = ? ";
            $params[] = $raw[1];
        }

        $sql = "SELECT $col FROM users WHERE " . implode(" OR ", $where);
        var_dump($sql, $params);
        $stmt = $pdo->prepare($sql);
        $stmt->execute($params);
        $user = $stmt->fetchAll(PDO::FETCH_ASSOC);

        foreach ($user as $row) {
            echo implode(' : ', $row) . PHP_EOL;
        }
        break;
    default:
        die("Invalid action");
}
