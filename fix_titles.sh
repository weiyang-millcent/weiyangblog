#!/bin/bash
echo "=== 为侧边栏卡片添加中文标题 ==="

# 备份配置
cp _config.butterfly.yml _config.butterfly.yml.backup

# 定义要添加的标题
declare -A titles=(
  ["card_announcement"]="公告"
  ["card_recent_post"]="最新文章"
  ["card_categories"]="分类"
  ["card_tags"]="标签"
  ["card_archives"]="归档"
  ["card_webinfo"]="网站信息"
  ["card_author"]="关于作者"
)

# 为每个卡片添加title字段
for card in "${!titles[@]}"; do
  if grep -q "^  $card:" _config.butterfly.yml; then
    echo "处理: $card → ${titles[$card]}"
    
    # 检查是否已有title字段
    if ! grep -q "^  $card:" -A5 _config.butterfly.yml | grep -q "title:"; then
      # 在enable行后添加title
      sed -i "/^  $card:/,/^  [a-z]/ { /^  $card:/ { n; /enable:/ { n; i\    title: ${titles[$card]}" _config.butterfly.yml; } }" _config.butterfly.yml
    else
      # 更新现有的title
      sed -i "/^  $card:/,/^  [a-z]/ s/^    title:.*/    title: ${titles[$card]}/" _config.butterfly.yml
    fi
  fi
done

echo "修复完成！"
