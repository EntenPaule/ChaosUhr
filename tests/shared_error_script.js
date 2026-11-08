const fs=require('node:fs'),path=require('node:path');
module.exports=()=>fs.readFileSync(path.join(__dirname,'../include/error_ui.h'),'utf8').match(/R"CHAOSJS\(([\s\S]*)\)CHAOSJS"/)[1];
