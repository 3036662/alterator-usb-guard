for file in $(find . -name "*.cpp" -o -name "*.hpp"); do
    if [ -f "$file" ]; then
		clang-format -i $file
    fi
done
