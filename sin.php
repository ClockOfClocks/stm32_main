<?php

$count = 256; // 64 microsteps
$max = pow(2 , 10);

for($i=0;$i<$count;$i++){
    $sin = number_format(sin(( 2* M_PI) * $i / $count ), 5);
    $cos = number_format(cos(( 2* M_PI) * $i / $count ), 5);
    $value = (int) ($sin * $max);
    //echo $sin . ' | ' . $value . ' | ' . (int)($cos * $max) . PHP_EOL;

    echo ($i > 0 ? ',' : '') . $value;
    if($i > 0 && $i % 16 == 0){
        echo PHP_EOL;
    }
}