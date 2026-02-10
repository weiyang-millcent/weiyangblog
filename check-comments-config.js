const fs = require('fs');
const yaml = require('js-yaml');

console.log('🔍 === 开始评论系统配置诊断 ===\n');

// 检查的文件列表
const configFiles = [
  '_config.yml',
  '_config.butterfly.yml',
  'themes/butterfly/_config.yml',
  'source/_data/butterfly.yml'
];

configFiles.forEach(file => {
  try {
    if (fs.existsSync(file)) {
      console.log(`📄 检查文件: ${file}`);
      const content = fs.readFileSync(file, 'utf8');
      const config = yaml.load(content);
      
      // 检查comments配置
      if (config.comments) {
        console.log(`   ✅ 找到comments配置:`);
        console.log(`      use: ${config.comments.use || '未设置'}`);
        console.log(`      text: ${config.comments.text || '未设置'}`);
        console.log(`      lazyload: ${config.comments.lazyload || '未设置'}`);
        
        if (config.comments.giscus) {
          console.log(`      giscus配置: 存在`);
        }
      } else {
        console.log(`   ❌ 没有comments配置`);
      }
      
      // 检查post配置
      if (config.post) {
        console.log(`   ✅ 找到post配置:`);
        console.log(`      comments: ${config.post.comments || '未设置'}`);
      }
      
      console.log('');
    }
  } catch (e) {
    console.log(`   ⚠️ 读取失败: ${e.message}`);
  }
});

console.log('✅ === 诊断结束 ===');
