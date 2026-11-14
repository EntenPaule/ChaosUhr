const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path');
const elements={};const context={document:{getElementById:id=>elements[id]??={textContent:''}}};
vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);
const count=()=>vm.runInContext('chaosBrowserErrors.length',context);
for(let i=0;i<20;i++)context.chaosMessage(context.chaosFailure(i%2?'FS_MOUNT':'HTTP_NETWORK'));
assert.equal(count(),8);assert.match(elements.browserDiagnostics.textContent,/FS-001/);assert.match(elements.browserDiagnostics.textContent,/Maßnahmen:/);
const text=context.chaosMessage(Object.assign(Error('fixture-private-password'),{chaosCode:'fixture-private-token'}));
assert(!/fixture|SYS-|Maßnahmen/.test(text));assert(!elements.browserDiagnostics.textContent.includes('fixture-private'));
const security=context.chaosFailure('AUTH_REJECTED');context.chaosMessage(security);
context.chaosMessage(context.chaosFailure('AUTH_SESSION_INVALID')); // Unclassified: preserve.
const expected=context.chaosFailure('AUTH_SESSION_EXPIRED');expected.expectedSession=true;context.chaosMessage(expected);
context.chaosResolveSessionErrors();assert(!elements.browserDiagnostics.textContent.includes('AUTH-020'));
assert(elements.browserDiagnostics.textContent.includes('AUTH-002'));assert(elements.browserDiagnostics.textContent.includes('AUTH-021'));assert(elements.browserDiagnostics.textContent.includes('FS-001'));
const initial=context.chaosFailure('AUTH_SESSION_INVALID');initial.quietSession=true;initial.expectedSession=true;
const before=count();assert.equal(context.chaosMessage(initial),'');assert.equal(count(),before);
for(const name of ['src/main.cpp','include/public_ui.h','include/ota_ui.h','include/portal_ui.h']){
 const source=fs.readFileSync(path.join(__dirname,'..',name),'utf8');
 assert(source.includes('browserDiagnostics'),name);assert(!/chaosFailure\([^\n]*?\)\.message/.test(source),name);
 assert(!/textContent\s*=\s*(error|e)\.message/.test(source),name);
}
console.log('PASS central diagnostics: eight-entry browser ring, safe fixed details only under diagnosis, targeted lifecycle cleanup, security/other errors retained, silent initial state and all GUI surfaces');
