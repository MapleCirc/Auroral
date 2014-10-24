#!/bin/bash

# Git-Post 
# Arthur  : floatJ.090227
# Version : 0.9
# Usage   : 自動Diff並且Post到特定看板上面)
# Call by : 這支程式會由 ~/src/.git/hooks/post-commit 呼叫
# Setting : ~/src as a git res.
# Required:
#           - Git(廢話顆) + post-commit with executive perm
#           - 可運作的 Maple-itoc 相容版本 科科
#           - add_post
#           (an api for post article, you can also just copy the function from source code)


# ~/src/util/git_post/gitpost.sh

# 定義變數和常數
BRD_NAME="SVN_log"
TITLE="Maple-itoc tcirc BBS git: Rev "
AUTHOR="Git_Logger"
# NICK=`cat gitpost_big5_str_nick.inc`
NICK="尼克"

TMP_DIR="/home/bbs/src/util/gitpost"
REC_PATH="/home/bbs/src/util/gitpost/rev.rec"
ADDPOST_PATH="/home/bbs/bin/addpost"



# 檢查 TMP_DIR 是否存在
if [ "$TMP_DIR" == "" -o ! -d "$TMP_DIR" ]; then
  echo -e "\033[1;31m[錯誤]\033[m暫存目錄(TMP_DIR) $TMP_DIR 不存在，請手動檢查。"
  echo -e "\033[1;33m[EXIT] Git-Post EXIT: 1 \033[m"
  exit 1
fi

# 檢查 REC_PATH 是否存在
if [ "$REC_PATH" == "" -o ! -f "$REC_PATH" ]; then
  # 不存在，直接略過讀取
  echo -e "版本號紀錄檔(REC_PATH) $REC_PATH 不存在，將略過讀取。"
  echo ""
  echo "請輸入版本號： (將會以您輸入的版本號繼續動作)"
  read counter
  echo -e "您輸入的版本號紀錄：\033[1;33m$counter\033[m"
else
  # 存在的話，讀入到 counter
  counter=`cat $REC_PATH | grep rev | awk '{print $2}'`
  echo -e "版本號紀錄檔(REC_PATH) $REC_PATH 存在"
  echo -e "讀取到的(上一次的)版本號紀錄為：\033[1;33m$counter\033[m"
fi

# [TODO] counter++
counter=$(($counter+1))
echo -e "新的版本號為：\033[1;33m$counter\033[m"



# floatJ.090227: 容錯機制改用錯誤訊息取代。
git tag "rev_"$counter >& $TMP_DIR"/"git_tag_stat.tmp
result=`cat $TMP_DIR"/"git_tag_stat.tmp`


if [ "$result" == "" ]; then
  echo "版本號標籤新增成功"
  echo -e "這一版的標籤為：  \033[1;33mrev_$counter\033[m"
  echo ""
else
  echo -e "\033[1;31m[錯誤]\033[m 版本號標籤新增失敗，可能是由於同樣的標籤已存在。"
  echo $result
  echo -e "\033[1;33m[注意]\033[m 請修正 \033[1;33m$REC_PATH\033[m 內紀錄的版本號。"
  echo " 　　  修正完成後，請手動執行 gitpost。"
  echo -e "\033[1;33m[EXIT] Git-Post EXIT: 1 \033[m"

  exit 1
fi


git show > $TMP_DIR"/"git_log.tmp

#exit 0
# 最後，
# 發表文章到 BRD
# ~/src/util/addpost 板名 檔名 標題 作者 暱稱
echo -e "\033[1;36m[發表文章]\033[m$ADDPOST_PATH $BRD_NAME $TMP_DIR"/"git_log.tmp "\"$TITLE $counter\"" $AUTHOR $NICK"

# 這邊 VIM 色彩辨識會失敗 = =+ #"
$ADDPOST_PATH $BRD_NAME $TMP_DIR"/"git_log.tmp "$TITLE $counter" $AUTHOR $NICK "-h"

echo -e "\033[1m文章發表完成！\033[m"



# 把新的counter 在新增成功後才寫回檔案
echo "正在把新的版本號標籤寫回紀錄檔..."
printf "rev     $counter" > $REC_PATH


echo -e "\033[1;36m[EXIT] Git-Post EXIT: 0 \033[m"
#"

# 2009.02.27-28 [ TODO ]
# [SOLVED] 1. 解決 git log 的 Big5 內容亂碼問題 (google)
# ???      2. 微調 addpost.c (新增一個參數作為判定是否自動增加檔頭之用)
# [DONE]   2.5 addpost.c 自動增加檔頭
# [DONE]   3. 對 gitpost 新增 addpost (上面ECHO的那段)
# ???      4. 實作版本號++並寫回REV.REC的機制
#          5. 實裝 Git-Post (+x hook post-commit)
#
exit 1
