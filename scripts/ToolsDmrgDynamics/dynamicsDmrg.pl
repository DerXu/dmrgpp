#!/usr/bin/perl

use strict;
use warnings;
use Utils;

my ($site,$site2,$what,$root,$operatorLabel) = @ARGV;
defined($operatorLabel) or die "USAGE: $0 site site2 what root operatorLabel\n";
Utils::checkRange($operatorLabel,"c","n","s+","s-","sz");

my %params = Utils::loadParams("params.txt");

my $matrixFermionSign = ($operatorLabel eq "c") ? -1 : 1;
my $templateInput = "inputTemplate.inp";

my $n = Utils::getLabel($templateInput,"TotalNumberOfSites=");
my $model = Utils::getLabel($templateInput,"Model=");
checkOperatorLabels($operatorLabel,$model);
my $hilbertSize = ($model eq "HeisenbergSpinOneHalf") ? 2 : 4;
my @spectral;

my $siteMin = ($site < $site2) ? $site : $site2;
my $siteMax = ($site < $site2) ? $site2 : $site;
my $output = "$root${siteMin}_$siteMax";
my $isDiagonal = ($site == $site2) ? 1 : 0;
my $dataRoot = "$root${siteMin}_${siteMax}";

my $bb = ($what & 2);
if ($bb && ($site2 >= $site)) {
	for (my $type=0;$type<4;$type++) {
		last if ($type>1 and $isDiagonal);
		my $input = createInput($n,$dataRoot,$siteMin,$siteMax,$type);
		my $retSystem = system("./dmrg -f $input &> out");
		if ($retSystem != 0) {
			modifyInput($input);
			$retSystem = system("./dmrg -f $input &> out");
			die "$0: $input failed\n" if ($retSystem != 0);
		}

		print STDERR "Type $type done\n";
	}
	print STDERR "Sites $site $site2 done\n";
}


$bb = ($what & 1);

my $result;

if ($bb  && ($site2 >= $site)) {
	$result = postProc($dataRoot,$siteMin,$siteMax);
}

die "$0: $result does not exist\n" unless (defined($result) and -r "$result");

print STDERR "$0: Done $site $site2\n";

sub modifyInput
{
	my ($input) = @_;

	my $fileinput = "file$input";
	open(FILE,$input) or die "$0: Cannot open $input : $!\n";
	open(FOUT,"> $fileinput") or die "$0: Cannot ope $fileinput for writing: $!\n";

	while(<FILE>) {
		if (/^TSPLoops/) {
			my @temp = split;
			print FOUT "$temp[0] $temp[1] ";
			my $total = scalar(@temp);
			for (my $i = 2; $i < $total; ++$i) {
				$_ = ($temp[$i] == 1) ? 3 : 1;
				print FOUT "$_ ";
			}

			print FOUT "\n";
			next;
		}

		print FOUT;
	}

	close(FILE);
	close(FOUT);
	system("cp $fileinput $input");
	system("sync; sleep 1");
	unlink("$fileinput");
}

sub postProc
{
	my ($dataRoot,$siteMin,$siteMax)=@_;

	my $energy = readEnergy("${dataRoot}_0.txt");
	print STDERR "Energy=$energy\n";

	my $isDiagonal = ($siteMin == $siteMax) ? 1 : 0;

	my $data = "";
	for (my $type=0;$type<4;$type++) {
		last if ($type>1 and $isDiagonal);
		my $dataTmp = "${dataRoot}_$type.txt";
		$data .= "$dataTmp ";
		unlink("EnvironStack$dataTmp");
		unlink("SystemStack$dataTmp");
		unlink("Wft$dataTmp");
	}

	my ($psimagLite,$begin,$end,$step,$eps) = ($params{"PsimagLite"},$params{"OmegaBegin"},$params{"OmegaEnd"},
		                                   $params{"OmegaStep"},$params{"OmegaEps"});
	system("$psimagLite/drivers/combineContinuedFraction $data > $dataRoot.comb");
	setEnergy("$dataRoot.comb",$energy);
	system("$psimagLite/drivers/continuedFractionCollection -f $dataRoot.comb -b -$begin -e $end -s $step -d $eps > $dataRoot.cf");

	my @temp = split/ /,$data;
	foreach (@temp) {
		unlink ($_) if (/$dataRoot/);
	}

	return "$dataRoot.cf";
}

sub createInput
{
	my ($n,$dataRoot,$site1,$site2,$type)=@_;
	my @sites = ($site1,$site2);
	my $file="input${site1}_${site2}_$type.inp";
	open(FOUT,">$file") or die "$0: Cannot write to $file\n";
	my ($sites,$loops,@matrix) = calcSitesAndLoops($n,$type,@sites);
	my $U = Utils::getLabel($templateInput,"##U=");
	my $hubbardU = setVector($n,$U);
	my $V = Utils::getLabel($templateInput,"##V=");
	my $potentialV = setVector(2*$n,$V);
	my $data = "${dataRoot}_${type}.txt";
	my $steps = int($n/2) - 1;

	open(FILE,"$templateInput") or die "$0: Cannot open $templateInput: $!\n";

	while(<FILE>) {
		next if (/^#/);
		if (/\$([a-zA-Z0-9\[\]]+)/) {
				my $name = $1;
				my $str = "\$".$name;
				my $val = eval "$str";
				defined($val) or die "$0: Undefined substitution for $name\n";
				s/\$\Q$name/$val/g;
		}
		print FOUT;
	}

	close(FILE);
	close(FOUT);

	return $file;
}

sub calcSitesAndLoops
{
	my ($n,$type,@sites)=@_;
	my ($sites,$loops,$matrix) = calcSitesAndLoopsAux($n,$type,@sites);
	
	return reorderSites($sites,$loops,$matrix);
}

sub reorderSites
{
	my ($sites,$loops,$matrix) = @_;
	my @sites2 = @$sites;
	my $nsites = scalar(@sites2);

	my @perm = sort { $sites2[$a] <  $sites2[$b] } 0 .. @sites2-1;

	my @newsites;
	my @newloops;
	my @newmatrix;

	for (my $i = 0; $i < 4; $i++) {
		$newmatrix[$i] = zeroMatrix();
	}

	for (my $i = 0; $i < $nsites; $i++) {
		my $j = $i;
		$newsites[$i] = $sites2[$perm[$j]];
		$newloops[$i] = $loops->[$perm[$j]];
		$newmatrix[$i] = $matrix->[$perm[$j]];
	}

	my $newsites = "$nsites ".arrayToString(\@newsites);
	my $newloops = "$nsites ".arrayToString(\@newloops);
	my $nOver2 = int($n/2);

	if ($newsites[0] == $n-1) {
		my $indexToSwap = 1;
		$indexToSwap = 2 if ($nsites == 3 and $newsites[2] >= $nOver2);

		my ($tmp1,$tmp2) = ($newsites[0],$newsites[$indexToSwap]);
		Utils::myswap(\$tmp1,\$tmp2);
		($newsites[0],$newsites[$indexToSwap]) = ($tmp1,$tmp2);
		$newsites = "$nsites ".arrayToString(\@newsites);

		Utils::setVector(\@newloops,0);
		$newloops = "$nsites ".arrayToString(\@newloops);

		($tmp1,$tmp2) = ($newmatrix[0], $newmatrix[$indexToSwap]);
		Utils::myswap(\$tmp1,\$tmp2);
		($newmatrix[0], $newmatrix[$indexToSwap]) = ($tmp1,$tmp2);
	}

	return ($newsites,$newloops,@newmatrix);
}

sub arrayToString
{
	my ($a) = @_;
	my $total = scalar(@$a);
	my $str = "";
	for (my $i = 0; $i < $total; $i++) {
		$str .= "$a->[$i] ";
	}
	return $str;
}

sub calcSitesAndLoopsAux
{
	my ($n,$type,@sites)=@_;
	my $loopDelay = 1;
	my @sites2 = @sites;
	my @loops = ($loopDelay, $loopDelay);
	my $isDiagonal = ($sites[0] == $sites[1]) ? 1 : 0;
	@sites2 = ($sites[0]) if ($isDiagonal);
	@loops = ($loopDelay) if ($isDiagonal);

	my @matrix;
	$matrix[0] = findMatrix($type,0);
	$matrix[1] = findMatrix($type,1);
	$matrix[2] = zeroMatrix();
	$matrix[3] = zeroMatrix();
	my $isOk1 = ($sites[0] != 0 and $sites[1] != $n-1) ? 1 : 0;
	my $isOk2 = ($sites[0] == 0 and $sites[1] == 1) ? 1 : 0;
	my $isOk3 = ($sites[0] == $n-2 and $sites[1] == $n-1) ? 1 : 0;
	return (\@sites2,\@loops,\@matrix) if ($isOk1 || $isOk2 || $isOk3);

	if ($isDiagonal) {
		if ($sites[0] == 0) {
			@sites2 = (1, 0);
			@loops = ($loopDelay, $loopDelay);
			$matrix[0] = zeroMatrix();
			$matrix[1] = findMatrix($type,0);
			return (\@sites2,\@loops,\@matrix);
		}

		if ($sites[0] == $n - 1) {
			@sites2 = ($n-2,$n-1);
			@loops = ($loopDelay, $loopDelay);
			$matrix[0] = zeroMatrix();
		       	$matrix[1] = findMatrix($type,1);
			return (\@sites2,\@loops,\@matrix);
		}

		die "$0: calcSitesAndLoops\n";
	}

	if ($sites[0] == 0 && $sites[1] != $n - 1) {
		@sites2 = (1, $sites[0], $sites[1]);
		@loops = ($loopDelay, $loopDelay, $loopDelay);
		$matrix[0] = zeroMatrix();
		$matrix[1] = findMatrix($type,0);
		$matrix[2] = findMatrix($type,1);
		return (\@sites2,\@loops,\@matrix);
	}

	if ($sites[0] != 0 && $sites[1] == $n - 1) {
		@sites2 = ($sites[0], $n-2, $sites[1]);
		@loops = ($loopDelay, $loopDelay, $loopDelay);
		$matrix[0] = findMatrix($type,0);
		$matrix[1] = zeroMatrix();
		$matrix[2] = findMatrix($type,1);
		return (\@sites2,\@loops,\@matrix);
	}

	die "$0: calcSitesAndLoops\n" unless ($sites[0] == 0 and $sites[1] == $n-1);

	@sites2 = ($sites[0], 1, $n-2, $sites[1]);
	@loops = ($loopDelay, $loopDelay, $loopDelay, $loopDelay);
	$matrix[0] = findMatrix($type,0);
	$matrix[1] = zeroMatrix();
	$matrix[2] = zeroMatrix();
	$matrix[3] = findMatrix($type,1);
	return (\@sites2,\@loops,\@matrix);
}

sub findMatrix
{
	my ($type,$type2)=@_;
	my $m = findMatrixAux($type,$type2);
	my $matrixEnd = "FERMIONSIGN=$matrixFermionSign\nJMVALUES 0 0\nAngularFactor=1\n\n";
	return "$m$matrixEnd";
}

sub findMatrixAux
{
	my ($type,$type2)=@_;
	if ($model eq "HeisenbergSpinOneHalf") {
		return findMatrixHeisenbergSpinOneHalf($type,$type2);
	}

	findMatrixHubbard($type,$type2);
}

sub findMatrixHeisenbergSpinOneHalf
{
	my ($type,$type2)=@_;
	my $matrix = ($operatorLabel eq "sz") ? "1 0\n0 -1\n"
                                    : "0 0\n1 0\n";
	if (($type & 1) && !($operatorLabel eq "sz")) {
		$matrix = "0 1\n0 0\n";
	}

	return $matrix if ($type2==0);

	if ($type>1) {
		$matrix = multiplyMatrix($matrix,-1);
	}

	return $matrix;
}

sub findMatrixHubbard
{
	my ($type,$type2)=@_;
	
	my $matrix = ($operatorLabel eq "c") ? "0 1 0 0\n0 0 0 0\n0 0 0 1\n0 0 0 0\n"
                                    : "0 0 0 0\n0 1 0 0\n0 0 0 0\n0 0 0 1\n";
	if (($type & 1) && ($operatorLabel eq "c")) {
		 $matrix = "0 0 0 0\n1 0 0 0\n0 0 0 0\n0 0 1 0\n";
	}

	return $matrix if ($type2==0);

	if ($type>1) {
		$matrix = multiplyMatrix($matrix,-1);
	}

	return $matrix;
}

sub zeroMatrix
{
	my $id =  "";
	for (my $i = 0; $i < $hilbertSize; ++$i) {
		for (my $j = 0; $j < $hilbertSize; ++$j) {
			$id .= "0 ";
		}
		$id .= "\n";
	}

	my $matrixEnd = "FERMIONSIGN=1\nJMVALUES 0 0\nAngularFactor=1\n\n";
        return "$id$matrixEnd";
}
sub multiplyMatrix
{
	my ($matrix)=@_;
	$matrix=~s/\n/ /g;
	my @temp=split(/ /,$matrix);
	my $matrix2="";
	for (my $i=0;$i<scalar(@temp);$i++) {
		$_ = -$temp[$i];
		$matrix2 .= $_." ";
	}
	return $matrix2;
}

sub setVector
{
	my ($sites,$U)=@_;
	my $tmp = "$sites ";
	for (my $i=0;$i<$sites;$i++) {
		$tmp .= " $U ";
	}
	return $tmp;
}

sub setEnergy
{
	my ($file,$energy)=@_;
	open(FILE,"$file") or die "$0: Cannot open $file\n";
	open(FOUT,">tmp.comb") or die "$0: Cannot write to tmp.comb\n";
	while(<FILE>) {
		if (/#CFEnergy=/) {
			print FOUT "#CFEnergy=$energy\n";
			next;
		}
		print FOUT;
	}
	close(FILE);
	close(FOUT);
	system("cp tmp.comb data.comb");
}

sub readEnergy
{
	my ($file)=@_;
	open(FILE,"$file") or die "$0: Cannot open $file\n";
	my $energy = 0;
	my $prevEnergy;
	while(<FILE>) {
		chomp;
		if (/#Energy=(.*$)/) {
			$energy=$1;
			if (defined($prevEnergy) and abs($energy-$prevEnergy)<1e-3) {
				last;
			}
			$prevEnergy = $energy;
		}
	}
	close(FILE);
	return $energy;
}

sub checkOperatorLabels
{
	my ($operatorLabel,$model) = @_;
	if ($model eq "HeisenbergSpinOneHalf") {
		return if ($operatorLabel eq "sz" or $operatorLabel eq "s+" or $operatorLabel eq "s-");
		die "$0: $operatorLabel not valid for $model\n";
	} else {
		return if ($operatorLabel eq "c" or $operatorLabel eq "n");
	}

	die "$0: $operatorLabel not valid for $model\n";
}

