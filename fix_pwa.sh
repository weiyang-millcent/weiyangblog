#!/bin/bash
echo "正在修复PWA插件问题..."

# 1. 备份
cp _config.yml _config.yml.bak
cp package.json package.json.bak

# 2. 卸载插件
npm uninstall hexo-pwa 2>/dev/null

# 3. 删除package.json中的引用
sed -i '/"hexo-pwa"/d' package.json

# 4. 注释_config.yml中的pwa配置
if grep -q "^pwa:" _config.yml; then
  sed -i '/^pwa:/,/^[^ ]/s/^/# /' _config.yml
  echo "已注释_config.yml中的pwa配置"
fi

# 5. 清理
rm -rf node_modules/hexo-pwa 2>/dev/null

echo "修复完成！"
