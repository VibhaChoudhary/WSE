#!/bin/bash
#wet_path
wet_path="D://wet.paths"
#page path
data="D:/pages"
#tika ja path
tika_path="D://tika-app-1.18.jar"
base_url="https://commoncrawl.s3.amazonaws.com/"
get_pages() {
    root=`echo $1|cut -d'/' -f6|cut -d'.' -f1`
    if [ -d "$root" ]; then
      return;
    fi
    mkdir $root
    cd $root  
    wget $base_url$path -O temp.gz  
    #create hierarchical directory structure and store pages
    zcat temp.gz | awk -v root="$root" -v parent="$data" -v tika="$tika_path" '/WARC-Target-URI:/{
    flag=1;cnt=0;
    if (n%100 == 0){
        dir = n/100;
        new="d_"dir;
        system("mkdir "new"");
    }
    n++;page=new"/"n".txt";}
    /WARC-Type: c/{
    flag=0;
    close(page);
    page_path="file:/"parent"/"root"/"page
    cmd1 = "java -jar \""tika"\" -l \""page_path"\""
    cmd1 | getline lang
    close(cmd1)
    if (substr(lang,0,2) != "en"){system("rm "page"")}
    }{
    if (flag==1){
    print >> page;}cnt++;
    }'
    rm temp.gz 
}
set -e

cd "$data"
for path in `shuf -n 1 "$wet_path"`
do
    get_pages $path &
done    


