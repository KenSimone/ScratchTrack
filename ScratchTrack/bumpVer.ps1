(Get-Content -path $args[0] -Raw) |
    ForEach-Object {

        $year="#define YEAR " + (Get-Date).Year
        $defstr="#define YEAR *";
        $regex="$defstr(?<BuildVersion>\d*)";
        if($_ -match $regex) {
            $_ = $_ -replace $regex,"$($year)" 
        }

        $dayofyear="#define VER_BUILD " + (Get-Date).DayOfYear
        $defstr="#define VER_BUILD *";
        $regex="$defstr(?<BuildVersion>\d*)";
        if($_ -match $regex) {
            $_ = $_ -replace $regex,"$($dayofyear)" 
        }

        $year = (Get-Date).Year.ToString()  
        $year = $year.Substring($year.Length - 2)
        $dayofyear="#define VER_SUBMINOR " + $year
        $defstr="#define VER_SUBMINOR *";
        $regex="$defstr(?<BuildVersion>\d*)";
        if($_ -match $regex) {
            $_ = $_ -replace $regex,"$($dayofyear)" 
        }

        $_
    } |
    Out-File $args[0] -encoding ascii -nonewline
