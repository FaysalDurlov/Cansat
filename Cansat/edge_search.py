from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys
import time

# ===== WORD LIST =====
words = [
    "climate change",
    "global warming",
    "environmental protection",
    "renewable energy",
    "solar power",
    "wind energy",
    "air pollution",
    "water pollution",
    "plastic waste",
    "recycling",
    "sustainable development",
    "green technology",
    "carbon footprint",
    "deforestation",
    "wildlife conservation",
    "biodiversity",
    "ecosystem balance",
    "climate action",
    "environmental awareness",
    "clean energy",
    "waste management",
    "ocean pollution",
    "tree plantation",
    "eco friendly lifestyle",
    "natural resources",
    "energy conservation",
    "environmental sustainability",
    "urban pollution",
    "climate resilience",
    "protect the planet"
]

# ===== EDGE OPTIONS =====
options = webdriver.EdgeOptions()
options.add_argument("--start-maximized")

driver = webdriver.Edge(options=options)

driver.get("https://www.bing.com")
time.sleep(3)

for word in words:
    search_box = driver.find_element(By.NAME, "q")

    search_box.clear()
    search_box.send_keys(word)
    time.sleep(2)          # wait after typing

    search_box.send_keys(Keys.ENTER)
    time.sleep(4)          # wait after search

print("All searches completed!")
time.sleep(3)     # optional: see last page
driver.quit()     # closes Edge + kills driver completely
