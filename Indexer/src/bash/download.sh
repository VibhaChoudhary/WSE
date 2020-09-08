#!/bin/bash
#wet_path
wet_path="D://wet.paths"
#destination path
data="D:/page"
#wet base url
base_url="https://commoncrawl.s3.amazonaws.com/"

get_pages() {
    #create directory for each wet file
    root=`echo $1|cut -d'/' -f6|cut -d'.' -f1`
    if [ -d "$root" ]; then
      return;
    fi
    mkdir $root
    cd $root  
    curl --url $base_url$1 --output temp.gz  
    #create hierarchical directory structure and store pages
    zcat temp.gz | awk '/WARC-Target-URI:/{
        flag = 1;
        cnt = 0;
        if (n%210 == 0){
            dir = n/210;
            new = "d_"dir;
            system("mkdir "new"");
        }
        n++;
        page = new"/"n".txt";
    }
    /WARC\/1.0/{
        flag = 0;
        close(page);            
    }
    {
        if (flag == 1){
            print >> page;
        }
        cnt++;
    }'
    rm temp.gz 
}

set -e
cd "$data"
get_pages "crawl-data/CC-MAIN-2019-39/segments/1568514570830.42/wet/CC-MAIN-20190915072355-20190915094355-00182.warc.wet.gz"

# #select random wet files and download pages
# for path in `shuf -n 5 "$wet_path"`
# do
    # get_pages $path &
# done    


